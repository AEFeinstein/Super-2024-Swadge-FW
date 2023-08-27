//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include "esp_timer.h"
#include "esp_log.h"
#include "hdw-nvs.h"
#include "hdw-usb.h"
#include "hid.h"
#include "menu.h"

#include "swadge2024.h"

#include "gamepad.h"
#include "mainMenu.h"

//==============================================================================
// Defines
//==============================================================================

#define Y_OFF 20

#define DPAD_BTN_RADIUS     16
#define DPAD_CLUSTER_RADIUS 45

#define START_BTN_RADIUS 10
#define START_BTN_SEP     2

#define AB_BTN_RADIUS 25
#define AB_BTN_Y_OFF   8
#define AB_BTN_SEP     2

#define ACCEL_BAR_HEIGHT  8
#define ACCEL_BAR_SEP     1
#define MAX_ACCEL_BAR_W 100

#define TOUCHBAR_WIDTH       100
#define TOUCHBAR_HEIGHT       20
#define TOUCHBAR_Y_OFF        55
#define TOUCHBAR_ANALOG_HEIGHT 8

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    GAMEPAD_MENU,
    GAMEPAD_MAIN
} gamepadScreen_t;

typedef enum {
    GAMEPAD_GENERIC,
    GAMEPAD_NS
} gamepadType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct // 4 bools = 4 bytes = 32 bits
{
    bool touchAnalogOn; // Least significant byte
    bool accelOn;
    bool _reserved1;
    bool _reserved2; // Most significant byte
} gamepadToggleSettings_t;

union gamepadToggleSettings_u
{
    int32_t i;
    gamepadToggleSettings_t settings;
};

typedef struct
{
    font_t ibmFont;
    font_t logbookFont;

    menu_t* menu;
    menuLogbookRenderer_t* renderer;
    gamepadScreen_t screen;
    uint16_t settingsPos;

    hid_gamepad_report_t gpState;
    hid_gamepad_ns_report_t gpNsState;

    uint8_t gamepadType;
    bool isPluggedIn;

    union gamepadToggleSettings_u gamepadToggleSettings;
} gamepad_t;

//==============================================================================
// Functions Prototypes
//==============================================================================

void gamepadEnterMode(void);
void gamepadExitMode(void);
void gamepadMainLoop(int64_t elapsedUs);
void gamepadButtonCb(buttonEvt_t* evt);
void gamepadReportStateToHost(void);

void setGamepadMainMenu(bool resetPos);
void gamepadMainMenuCb(const char* label, bool selected, uint32_t settingVal);
void gamepadMenuLoop(int64_t elapsedUs);
void gamepadStart(gamepadType_t type);

static bool saveGamepadToggleSettings(union gamepadToggleSettings_u* toggleSettings);
static bool loadGamepadToggleSettings(union gamepadToggleSettings_u* toggleSettings);

static const char* getButtonName(hid_gamepad_button_bm_t button);

//==============================================================================
// Variables
//==============================================================================

static const char str_pc[] = "PC";
static const char str_ns[] = "Switch";
static const char str_touch_analog_on[] = "Touch: Digi+Analog";
static const char str_touch_analog_off[] = "Touch: Digital Only";
static const char str_accel_on[] = "Accel: On";
static const char str_accel_off[] = "Accel: Off";
static const char str_exit[] = "Exit";
static const char KEY_GAMEPAD_SETTINGS[] = "gp_settings";

gamepad_t* gamepad;

swadgeMode_t gamepadMode =
{
    .modeName = "Gamepad",
    .wifiMode = NO_WIFI,
    .overrideUsb = true,
    .usesAccelerometer = true,
    .usesThermometer = false,
    .fnEnterMode = gamepadEnterMode,
    .fnExitMode = gamepadExitMode,
    .fnMainLoop = gamepadMenuLoop,
    .fnAudioCallback = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb = NULL,
    .fnEspNowSendCb = NULL,
    .fnAdvancedUSB= NULL
    
};

// TODO: handle 8-way joystick
const hid_gamepad_button_bm_t touchMap[] =
{
    GAMEPAD_BUTTON_C,
    GAMEPAD_BUTTON_X,
    GAMEPAD_BUTTON_Y,
    GAMEPAD_BUTTON_Z,
    GAMEPAD_BUTTON_TL,
};

// TODO: handle 8-way joystick?
const hid_gamepad_button_bm_t touchMapNs[] =
{
    GAMEPAD_NS_BUTTON_Y,
    GAMEPAD_NS_BUTTON_TL,
    GAMEPAD_NS_BUTTON_Z,
    GAMEPAD_NS_BUTTON_TR,
    GAMEPAD_NS_BUTTON_X,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * Enter the gamepad mode, allocate memory, intialize USB
 */
void gamepadEnterMode(void)
{
    // Allocate and zero memory
    gamepad = (gamepad_t*)calloc(1, sizeof(gamepad_t));
    
    // Load the fonts
    loadFont("logbook.font", &(gamepad->logbookFont), false);
    loadFont("ibm_vga8.font", &(gamepad->ibmFont), false);

    // Initialize menu
    gamepad->menu = initMenu(gamepadMode.modeName, gamepadMainMenuCb);

    loadGamepadToggleSettings(&gamepad->gamepadToggleSettings);

    // Setup menu items
    setGamepadMainMenu(true);

    // Initialize menu renderer
    gamepad->renderer = initMenuLogbookRenderer(&gamepad->logbookFont);
}

/**
 * Exit the gamepad mode and free memory
 */
void gamepadExitMode(void)
{
    deinitMenu(gamepad->menu);
    deinitMenuLogbookRenderer(gamepad->renderer);
    freeFont(&(gamepad->logbookFont));
    freeFont(&(gamepad->ibmFont));

    free(gamepad);
}

void setGamepadMainMenu(bool resetPos)
{
    deinitMenu(gamepad->menu);
    gamepad->menu = initMenu(gamepadMode.modeName, gamepadMainMenuCb);
    addSingleItemToMenu(gamepad->menu, str_pc);
    addSingleItemToMenu(gamepad->menu, str_ns);
    addSingleItemToMenu(gamepad->menu, gamepad->gamepadToggleSettings.settings.touchAnalogOn ? str_touch_analog_on : str_touch_analog_off);
    addSingleItemToMenu(gamepad->menu, gamepad->gamepadToggleSettings.settings.accelOn ? str_accel_on : str_accel_off);
    addSingleItemToMenu(gamepad->menu, str_exit);

    gamepad->screen = GAMEPAD_MENU;

    // Set the position
    if(resetPos)
    {
        gamepad->settingsPos = 0;
    }
    gamepad->menu->currentItem = gamepad->settingsPos;
}

void gamepadMainMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if(selected)
    {
        if (label == str_pc)
        {
            gamepadStart(GAMEPAD_GENERIC);
            gamepad->screen = GAMEPAD_MAIN;
            return;
        }
        else if (label == str_ns)
        {
            gamepadStart(GAMEPAD_NS);
            gamepad->screen = GAMEPAD_MAIN;
            return;
        }
        else if (label == str_exit)
        {
            // Exit to main menu
            switchToSwadgeMode(&mainMenuMode);
            return;
        }
    }
    else // TODO: verify this is the proper way to use `selected`
    {
        // Save the position
        gamepad->settingsPos = gamepad->menu->currentItem;

        bool needRedraw = false;

        if(label == str_touch_analog_on)
        {
            // Touch analog is on, turn it off
            gamepad->gamepadToggleSettings.settings.touchAnalogOn = false;
            saveGamepadToggleSettings(&gamepad->gamepadToggleSettings);

            needRedraw = true;
        }
        else if(label == str_touch_analog_off)
        {
            // Touch analog is off, turn it on
            gamepad->gamepadToggleSettings.settings.touchAnalogOn = true;
            saveGamepadToggleSettings(&gamepad->gamepadToggleSettings);

            needRedraw = true;
        }
        else if(label == str_accel_on)
        {
            // Accel is on, turn it off
            gamepad->gamepadToggleSettings.settings.accelOn = false;
            saveGamepadToggleSettings(&gamepad->gamepadToggleSettings);

            needRedraw = true;
        }
        else if(label == str_accel_off)
        {
            // Accel is off, turn it on
            gamepad->gamepadToggleSettings.settings.accelOn = true;
            saveGamepadToggleSettings(&gamepad->gamepadToggleSettings);

            needRedraw = true;
        }

        if(needRedraw)
        {
            setGamepadMainMenu(false);
        }
    }
}

void gamepadStart(gamepadType_t type){
    gamepad->gamepadType = type;

    tusb_desc_device_t nsDescriptor = {
        .bLength = 18U,
        .bDescriptorType = 1,
        .bcdUSB = 0x0200,
        .bDeviceClass = 0x00,
        .bDeviceSubClass = 0x00,
        .bDeviceProtocol = 0x00,
        .bMaxPacketSize0 = 64,
        .idVendor = 0x0f0d,
        .idProduct = 0x0092,
        .bcdDevice = 0x0100,
        .iManufacturer = 0x01,
        .iProduct = 0x02,
        .iSerialNumber = 0x03,
        .bNumConfigurations = 0x01
    };

    tinyusb_config_t default_cfg = {
    };

    tinyusb_config_t tusb_cfg = {
        .descriptor = &nsDescriptor
    };

    tinyusb_driver_install((gamepad->gamepadType==GAMEPAD_NS) ? &tusb_cfg :&default_cfg);

    gamepad->gpNsState.x = 128;
    gamepad->gpNsState.y = 128;
    gamepad->gpNsState.rx = 128;
    gamepad->gpNsState.ry = 128;

    led_t leds[CONFIG_NUM_LEDS];
    memset(leds, 0, sizeof(leds));
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * Call the appropriate main loop function for the screen being displayed
 *
 * @param elapsedUd Time.deltaTime
 */
void gamepadMenuLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};

    switch(gamepad->screen)
    {
        case GAMEPAD_MENU:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                gamepad->menu = menuButton(gamepad->menu, evt);
            }
            drawMenuLogbook(gamepad->menu, gamepad->renderer, elapsedUs);
            break;
        }
        case GAMEPAD_MAIN:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                gamepadButtonCb(&evt);
            }
            gamepadMainLoop(elapsedUs);
            break;
        }
            //No wifi mode stuff
    }
}

/**
 * Draw the gamepad state to the display when it changes
 *
 * @param elapsedUs unused
 */
void gamepadMainLoop(int64_t elapsedUs __attribute__((unused)))
{
    // Check if plugged in or not
    if(tud_ready() != gamepad->isPluggedIn)
    {
        gamepad->isPluggedIn = tud_ready();
    }

    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c213);

    // Always Draw some reminder text, centered
    const char reminderText[] = "Start + Select to Exit";
    int16_t tWidth = textWidth(&gamepad->ibmFont, reminderText);
    drawText(&gamepad->ibmFont, c555, reminderText, (TFT_WIDTH - tWidth) / 2, 10);

    if(gamepad->gamepadType == GAMEPAD_NS)
    {
        // Draw button combo text, centered
        const char captureText[] = "Down + Select:  Capture";
        tWidth = textWidth(&gamepad->ibmFont, captureText);
        int16_t textX = (TFT_WIDTH - tWidth) / 2;
        int16_t afterText = drawText(&gamepad->ibmFont, c555, captureText, textX, TFT_HEIGHT - gamepad->ibmFont.height * 2 - 12);

        const char homeText1[] = "Down + Start:";
        drawText(&gamepad->ibmFont, c555, homeText1, textX, TFT_HEIGHT - gamepad->ibmFont.height - 10);

        const char* homeText2 = getButtonName(GAMEPAD_NS_BUTTON_HOME);
        tWidth = textWidth(&gamepad->ibmFont, homeText2);
        drawText(&gamepad->ibmFont, c555, homeText2, afterText - tWidth - 1, TFT_HEIGHT - gamepad->ibmFont.height - 10);
    }

    // If it's plugged in, draw buttons
    if(gamepad->isPluggedIn)
    {
        // Helper function pointer
        void (*drawFunc)(int, int, int, paletteColor_t);

        // A list of all the hat directions, in order
        static const uint8_t hatDirs[] = 
        {
            GAMEPAD_HAT_UP,
            GAMEPAD_HAT_UP_RIGHT,
            GAMEPAD_HAT_RIGHT,
            GAMEPAD_HAT_DOWN_RIGHT,
            GAMEPAD_HAT_DOWN,
            GAMEPAD_HAT_DOWN_LEFT,
            GAMEPAD_HAT_LEFT,
            GAMEPAD_HAT_UP_LEFT
        };

        // For each hat direction
        for(uint8_t i = 0; i < ARRAY_SIZE(hatDirs); i++)
        {
            // The degree around the cluster
            int16_t deg = i * 45;
            // The center of the cluster
            int16_t xc = TFT_WIDTH / 4;
            int16_t yc = (TFT_HEIGHT / 2) + Y_OFF;
            // Draw the button around the cluster
            xc += (( getSin1024(deg) * DPAD_CLUSTER_RADIUS) / 1024);
            yc += ((-getCos1024(deg) * DPAD_CLUSTER_RADIUS) / 1024);

            // Draw either a filled or outline circle, if this is the direction pressed
            switch(gamepad->gamepadType){
                case GAMEPAD_NS:{
                    drawFunc = (gamepad->gpNsState.hat == (hatDirs[i]-1)) ? &drawCircleFilled : &drawCircle;
                    break;
                }
                case GAMEPAD_GENERIC:
                default: {
                    drawFunc = (gamepad->gpState.hat == hatDirs[i]) ? &drawCircleFilled : &drawCircle;
                    break;
                }
            }

            drawFunc(xc, yc, DPAD_BTN_RADIUS, c551 /*paletteHsvToHex(i * 32, 0xFF, 0xFF)*/);
        }

        // Select button
        switch(gamepad->gamepadType){
            case GAMEPAD_NS:{
                drawFunc = (gamepad->gpNsState.buttons & GAMEPAD_NS_BUTTON_MINUS) ? &drawCircleFilled : &drawCircle;
                break;
            }
            case GAMEPAD_GENERIC:
            default:{
                drawFunc = (gamepad->gpState.buttons & GAMEPAD_BUTTON_SELECT) ? &drawCircleFilled : &drawCircle;
                break;
            }
        }
        int16_t x = (TFT_WIDTH / 2) - START_BTN_RADIUS - START_BTN_SEP;
        int16_t y = (TFT_HEIGHT / 4) + Y_OFF;
        drawFunc(x, y, START_BTN_RADIUS, c333);

        if(gamepad->gamepadType == GAMEPAD_NS)
        {
            const char* buttonName = getButtonName(GAMEPAD_NS_BUTTON_MINUS);
            drawText(&gamepad->ibmFont, c444, buttonName, x - textWidth(&gamepad->ibmFont, buttonName) / 2, y - gamepad->ibmFont.height / 2);
        }

        // Start button
        switch(gamepad->gamepadType){ 
            case GAMEPAD_NS:{
                drawFunc = (gamepad->gpNsState.buttons & GAMEPAD_NS_BUTTON_PLUS) ? &drawCircleFilled : &drawCircle;
                break;
            }
            case GAMEPAD_GENERIC:
            default:{
                drawFunc = (gamepad->gpState.buttons & GAMEPAD_BUTTON_START) ? &drawCircleFilled : &drawCircle;
                break;
            }
        }
        x = (TFT_WIDTH / 2) + START_BTN_RADIUS + START_BTN_SEP;
        drawFunc(x,
                 y,
                 START_BTN_RADIUS, c333);

        if(gamepad->gamepadType == GAMEPAD_NS)
        {
            const char* buttonName = getButtonName(GAMEPAD_NS_BUTTON_PLUS);
            drawText(&gamepad->ibmFont, c444, buttonName, x - textWidth(&gamepad->ibmFont, buttonName) / 2, y - gamepad->ibmFont.height / 2);
        }

        // Button A
        switch(gamepad->gamepadType){
            case GAMEPAD_NS:{
                drawFunc = (gamepad->gpNsState.buttons & GAMEPAD_NS_BUTTON_A) ? &drawCircleFilled : &drawCircle;
                break;
            }
            case GAMEPAD_GENERIC:
            default: {
                drawFunc = (gamepad->gpState.buttons & GAMEPAD_BUTTON_A) ? &drawCircleFilled : &drawCircle;
                break;
            }
        }
        drawFunc(
                 ((3 * TFT_WIDTH) / 4) + AB_BTN_RADIUS + AB_BTN_SEP,
                 (TFT_HEIGHT / 2) - AB_BTN_Y_OFF + Y_OFF,
                 AB_BTN_RADIUS, c243);

        // Button B
        switch(gamepad->gamepadType){
            case GAMEPAD_NS:{
                drawFunc = (gamepad->gpNsState.buttons & GAMEPAD_NS_BUTTON_B) ? &drawCircleFilled : &drawCircle;
                break;
            }
            case GAMEPAD_GENERIC:
            default:{
                drawFunc = (gamepad->gpState.buttons & GAMEPAD_BUTTON_B) ? &drawCircleFilled : &drawCircle;
                break;
            }
        }
        drawFunc(
                 ((3 * TFT_WIDTH) / 4) - AB_BTN_RADIUS - AB_BTN_SEP,
                 (TFT_HEIGHT / 2) + AB_BTN_Y_OFF + Y_OFF,
                 AB_BTN_RADIUS, c401);

        // Draw touch strip
        int16_t tBarX = TFT_WIDTH - TOUCHBAR_WIDTH;

        // TOOD: translate new touchpad data into 4-way or 8-way joystick
        // switch(gamepad->gamepadType){
        //     case GAMEPAD_GENERIC: {
        //         if(evt->down)
        //         {
        //             gamepad->gpState.buttons |= touchMap[evt->pad];
        //         }
        //         else
        //         {
        //             gamepad->gpState.buttons &= ~touchMap[evt->pad];
        //         }

        //         break;
        //     }
        //     case GAMEPAD_NS: {
        //         if(evt->down)
        //         {
        //             gamepad->gpNsState.buttons |= touchMapNs[evt->pad];
        //         }
        //         else
        //         {
        //             gamepad->gpNsState.buttons &= ~touchMapNs[evt->pad];
        //         }

        //         break;
        //     }
        // }

        // If we're on the generic gamepad and touch analog is enabled, plot the extra indicator on the screen
        if(gamepad->gamepadType == GAMEPAD_GENERIC && gamepad->gamepadToggleSettings.settings.touchAnalogOn)
        {
            int32_t center, intensity;
            if(getTouchCentroid(&center, &intensity))
            {
                // TODO: rebuild this for new touchpad
                // Subtract 2 from TOUCHBAR_WIDTH while scaling so we can draw a 3px-wide cursor centered on center, without covering the box borders
                center = (TOUCHBAR_WIDTH - 2) * center / 1024 + 1;
            }
            else
            {
                center = TOUCHBAR_WIDTH / 2;
                // Intensity is unused, so no need to set right now. Uncomment if it gets used
                //intensity = 0;
            }

            drawRect(
                     tBarX - 1       , TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT                          - 1,
                     TFT_WIDTH, TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT + TOUCHBAR_ANALOG_HEIGHT + 1,
                     c111);
            fillDisplayArea(
                            tBarX + center - 1, TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT,
                            tBarX + center + 1, TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT + TOUCHBAR_ANALOG_HEIGHT,
                            c444);
        }

        uint8_t numTouchElem = ARRAY_SIZE(touchMap[0]);
        for(uint8_t touchIdx = 0; touchIdx < numTouchElem; touchIdx++)
        {
            int16_t x1 = tBarX - 1;
            int16_t x2 = tBarX + (TOUCHBAR_WIDTH / numTouchElem);

            if((gamepad->gamepadType == GAMEPAD_GENERIC) ? gamepad->gpState.buttons & touchMap[touchIdx]:gamepad->gpNsState.buttons & touchMapNs[touchIdx])
            {
                fillDisplayArea(
                                x1, TOUCHBAR_Y_OFF,
                                x2, TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT,
                                c111);
            }
            else
            {
                drawRect(
                         x1, TOUCHBAR_Y_OFF,
                         x2, TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT,
                         c111);

            }

            if(gamepad->gamepadType == GAMEPAD_NS)
            {
                const char* buttonName = getButtonName(touchMapNs[touchIdx]);
                drawText(&gamepad->ibmFont, c444, buttonName, x1 + (x2 - x1 - textWidth(&gamepad->ibmFont, buttonName)) / 2, TOUCHBAR_Y_OFF + (TOUCHBAR_HEIGHT - gamepad->ibmFont.height) / 2);
            }

            tBarX += (TOUCHBAR_WIDTH / numTouchElem);
        }

        if(gamepad->gamepadToggleSettings.settings.accelOn && gamepad->gamepadType == GAMEPAD_GENERIC)
        {
            // Declare variables to receive acceleration
            int16_t a_x, a_y, a_z;
            // Get the current acceleration
            if (ESP_OK == accelGetAccelVec(&a_x, &a_y, &a_z))
            {
                // Values are roughly -256 to 256, so divide, clamp, and save
                gamepad->gpState.rx = CLAMP((a_x) / 2, -128, 127);
                gamepad->gpState.ry = CLAMP((a_y) / 2, -128, 127);
                gamepad->gpState.rz = CLAMP((a_z) / 2, -128, 127);
            }

            // Set up drawing accel bars
            int16_t barY = (TFT_HEIGHT * 3) / 4;

            // Plot X accel
            int16_t barWidth = ((gamepad->gpState.rx + 128) * MAX_ACCEL_BAR_W) / 256;
            fillDisplayArea(TFT_WIDTH - barWidth, barY, TFT_WIDTH, barY + ACCEL_BAR_HEIGHT, c500);
            barY += (ACCEL_BAR_HEIGHT + ACCEL_BAR_SEP);

            // Plot Y accel
            barWidth = ((gamepad->gpState.ry + 128) * MAX_ACCEL_BAR_W) / 256;
            fillDisplayArea(TFT_WIDTH - barWidth, barY, TFT_WIDTH, barY + ACCEL_BAR_HEIGHT, c050);
            barY += (ACCEL_BAR_HEIGHT + ACCEL_BAR_SEP);

            // Plot Z accel
            barWidth = ((gamepad->gpState.rz + 128) * MAX_ACCEL_BAR_W) / 256;
            fillDisplayArea(TFT_WIDTH - barWidth, barY, TFT_WIDTH, barY + ACCEL_BAR_HEIGHT, c005);
            // barY += (ACCEL_BAR_HEIGHT + ACCEL_BAR_SEP);
        }

        // Send state to host
        gamepadReportStateToHost();
    }
    else
    {
        // If it's not plugged in, give a hint
        const char* plugInText;
        switch(gamepad->gamepadType){
            case GAMEPAD_NS:{
                plugInText = "Plug USB-C into Switch please!";
                break;
            }
            case GAMEPAD_GENERIC:
            default: {
                plugInText = "Plug USB-C into computer please!";
                break;
            }
        }
        tWidth = textWidth(&gamepad->ibmFont, plugInText);
        drawText(&gamepad->ibmFont, c555, plugInText,
                 (TFT_WIDTH - tWidth) / 2,
                 (TFT_HEIGHT - gamepad->ibmFont.height) / 2);
    }
}

/**
 * Button callback. Send the button state over USB and save it for drawing
 *
 * @param evt The button event that occurred
 */
void gamepadButtonCb(buttonEvt_t* evt)
{
    switch(gamepad->gamepadType){
        case GAMEPAD_GENERIC: {
            // Build a list of all independent buttons held down
            gamepad->gpState.buttons &= ~(GAMEPAD_BUTTON_A | GAMEPAD_BUTTON_B | GAMEPAD_BUTTON_START | GAMEPAD_BUTTON_SELECT);
            if(evt->state & PB_A)
            {
                gamepad->gpState.buttons |= GAMEPAD_BUTTON_A;
            }
            if(evt->state & PB_B)
            {
                gamepad->gpState.buttons |= GAMEPAD_BUTTON_B;
            }
            if(evt->state & PB_START)
            {
                gamepad->gpState.buttons |= GAMEPAD_BUTTON_START;
            }
            if(evt->state & PB_SELECT)
            {
                gamepad->gpState.buttons |= GAMEPAD_BUTTON_SELECT;
            }

            // Figure out which way the D-Pad is pointing
            gamepad->gpState.hat = GAMEPAD_HAT_CENTERED;
            if(evt->state & PB_UP)
            {
                if(evt->state & PB_RIGHT)
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_UP_RIGHT;
                }
                else if(evt->state & PB_LEFT)
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_UP_LEFT;
                }
                else
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_UP;
                }
            }
            else if(evt->state & PB_DOWN)
            {
                if(evt->state & PB_RIGHT)
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_DOWN_RIGHT;
                }
                else if(evt->state & PB_LEFT)
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_DOWN_LEFT;
                }
                else
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_DOWN;
                }
            }
            else if(evt->state & PB_RIGHT)
            {
                gamepad->gpState.hat = GAMEPAD_HAT_RIGHT;
            }
            else if(evt->state & PB_LEFT)
            {
                gamepad->gpState.hat = GAMEPAD_HAT_LEFT;
            }

            break;
        }
        case GAMEPAD_NS:{
            // Build a list of all independent buttons held down
            gamepad->gpNsState.buttons = 0;

            if(evt->state & PB_A)
            {
                gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_A;
            }
            if(evt->state & PB_B)
            {
                gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_B;
            }
            if(evt->state & PB_START)
            {
                if(evt->state & PB_DOWN){
                    gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_HOME;
                } else {
                    gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_PLUS;
                }
            }
            if(evt->state & PB_SELECT)
            {
                if(evt->state & PB_DOWN){
                    gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_CAPTURE;
                } else {
                    gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_MINUS;
                }
            }

            // Figure out which way the D-Pad is pointing
            gamepad->gpNsState.hat = GAMEPAD_NS_HAT_CENTERED;
            if(evt->state & PB_UP)
            {
                if(evt->state & PB_RIGHT)
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_UP_RIGHT;
                }
                else if(evt->state & PB_LEFT)
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_UP_LEFT;
                }
                else
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_UP;
                }
            }
            else if(evt->state & PB_DOWN)
            {
                if(evt->state & PB_RIGHT)
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_DOWN_RIGHT;
                }
                else if(evt->state & PB_LEFT)
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_DOWN_LEFT;
                }
                else
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_DOWN;
                }
            }
            else if(evt->state & PB_RIGHT)
            {
                gamepad->gpNsState.hat = GAMEPAD_NS_HAT_RIGHT;
            }
            else if(evt->state & PB_LEFT)
            {
                gamepad->gpNsState.hat = GAMEPAD_NS_HAT_LEFT;
            }

            break;
        }
    }
    
    // Send state to host
    gamepadReportStateToHost();
}

/**
 * @brief Send the state over USB to the host
 */
void gamepadReportStateToHost(void)
{
    // Only send data if USB is ready
    if(tud_ready())
    {
        switch(gamepad->gamepadType){
            case GAMEPAD_GENERIC: {
                // TODO: handle 8-way joystick
                if(gamepad->gamepadToggleSettings.settings.touchAnalogOn && gamepad->gpState.buttons & ((touchMap[0] | touchMap[1] | touchMap[2] | touchMap[3])))// | touchMap[4] | touchMap[5] | touchMap[6] | touchMap[7])))
                {
                    int32_t center, intensity;
                    // TODO: handle when this returns false
                    getTouchCentroid(&center, &intensity);
                    int16_t scaledVal = (center >> 2) - 128;
                    if(scaledVal < -128)
                    {
                        gamepad->gpState.z = -128;
                    }
                    else if (scaledVal > 127)
                    {
                        gamepad->gpState.z = 127;
                    }
                    else
                    {
                        gamepad->gpState.z = scaledVal;
                    }
                }
                else
                {
                    gamepad->gpState.z = 0;
                }
                // Send the state over USB
                tud_gamepad_report(&gamepad->gpState);
                break;
            }
            case GAMEPAD_NS: {
                tud_gamepad_ns_report(&gamepad->gpNsState);
                break;
            }
        }
        
        
    }    
}

static bool saveGamepadToggleSettings(union gamepadToggleSettings_u* toggleSettings)
{
    return writeNvs32(KEY_GAMEPAD_SETTINGS, toggleSettings->i);
}

static bool loadGamepadToggleSettings(union gamepadToggleSettings_u* toggleSettings)
{
    bool r = readNvs32(KEY_GAMEPAD_SETTINGS, &toggleSettings->i);
    if (!r)
    {
        memset(toggleSettings, 0, sizeof(union gamepadToggleSettings_u));
        toggleSettings->settings.accelOn = true;
        toggleSettings->settings.touchAnalogOn = true;
        return saveGamepadToggleSettings(toggleSettings);
    }
    return true;
}

static const char* getButtonName(hid_gamepad_button_bm_t button)
{
    switch(button)
    {
        case GAMEPAD_NS_BUTTON_Y:
        {
            return "Y";
        }
        case GAMEPAD_NS_BUTTON_B:
        {
            return "B";
        }
        case GAMEPAD_NS_BUTTON_A:
        {
            return "A";
        }
        case GAMEPAD_NS_BUTTON_X:
        {
            return "X";
        }
        case GAMEPAD_NS_BUTTON_TL:
        {
            return "L";
        }
        case GAMEPAD_NS_BUTTON_TR:
        {
            return "R";
        }
        case GAMEPAD_NS_BUTTON_TL2:
        {
            return "ZL";
        }
        case GAMEPAD_NS_BUTTON_TR2:
        {
            return "ZR";
        }
        case GAMEPAD_NS_BUTTON_MINUS:
        {
            return "-";
        }
        case GAMEPAD_NS_BUTTON_PLUS:
        {
            return "+";
        }
        case GAMEPAD_NS_BUTTON_HOME:
        {
            return "HOME";
        }
        case GAMEPAD_NS_BUTTON_CAPTURE:
        {
            return "Capture";
        }
        case GAMEPAD_NS_BUTTON_THUMBL:
        {
            return "Left Stick";
        }
        case GAMEPAD_NS_BUTTON_THUMBR:
        {
            return "Right Stick";
        }
        case GAMEPAD_NS_BUTTON_Z:
        default:
        {
            return "";
        }
    }
}
