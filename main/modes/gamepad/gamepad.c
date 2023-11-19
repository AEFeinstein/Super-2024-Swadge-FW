//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include "hdw-imu.h"
#include "tinyusb.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#include "touchTest.h"
#include "touchUtils.h"
#include "gamepad.h"
#include "mainMenu.h"
#include "settingsManager.h"
#include "swadge2024.h"

//==============================================================================
// Defines
//==============================================================================

#define Y_OFF 20

#define DPAD_BTN_RADIUS     16
#define DPAD_CLUSTER_RADIUS 45
#define DPAD_CLUSTER_Y_OFF  30

#define START_BTN_RADIUS 10
#define START_BTN_Y_OFF  50
#define START_BTN_SEP    2

#define AB_BTN_RADIUS 25
#define AB_BTN_Y_OFF  15
#define AB_BTN_Y_SEP  8
#define AB_BTN_SEP    2

#define ACCEL_BAR_HEIGHT 8
#define ACCEL_BAR_SEP    1
#define MAX_ACCEL_BAR_W  100

#define TOUCHPAD_DIAM  80
#define TOUCHPAD_Y_OFF 8
#define TOUCHPAD_X_OFF 10

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    GAMEPAD_MENU,
    GAMEPAD_MAIN
} gamepadScreen_t;

typedef enum
{
    GAMEPAD_GENERIC,
    GAMEPAD_NS
} gamepadType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    font_t ibmFont;
    font_t logbookFont;

    menu_t* menu;
    menuLogbookRenderer_t* renderer;
    gamepadScreen_t screen;

    hid_gamepad_report_t gpState;
    hid_gamepad_ns_report_t gpNsState;

    uint8_t gamepadType;
    bool isPluggedIn;

    int64_t exitTimer;
} gamepad_t;

//==============================================================================
// Functions Prototypes
//==============================================================================

void gamepadEnterMode(void);
void gamepadExitMode(void);
void gamepadMainLoop(int64_t elapsedUs);
void gamepadButtonCb(buttonEvt_t* evt);
void gamepadReportStateToHost(void);

void gamepadMainMenuCb(const char* label, bool selected, uint32_t settingVal);
void gamepadMenuLoop(int64_t elapsedUs);
void gamepadStart(gamepadType_t type);

static const char* getButtonName(hid_gamepad_button_bm_t button);

//==============================================================================
// Variables
//==============================================================================

static const char str_pc[]    = "Computer";
static const char str_ns[]    = "Switch";
static const char str_accel[] = "Accel: ";
static const char str_exit[]  = "Exit";

static const int32_t accelSettingsValues[] = {0, 1};

static const char* const accelSettingsOptions[] = {
    "Off",
    "On",
};

gamepad_t* gamepad;

swadgeMode_t gamepadMode = {
    .modeName                 = "Gamepad",
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = true,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .overrideSelectBtn        = true,
    .fnEnterMode              = gamepadEnterMode,
    .fnExitMode               = gamepadExitMode,
    .fnMainLoop               = gamepadMenuLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,

};

const hid_gamepad_button_bm_t touchMap[] = {
    GAMEPAD_BUTTON_C, GAMEPAD_BUTTON_X, GAMEPAD_BUTTON_Y, GAMEPAD_BUTTON_Z, GAMEPAD_BUTTON_TL,
};

const hid_gamepad_button_bm_t touchMapNs[] = {
    GAMEPAD_NS_BUTTON_Y, GAMEPAD_NS_BUTTON_TL, GAMEPAD_NS_BUTTON_Z, GAMEPAD_NS_BUTTON_TR, GAMEPAD_NS_BUTTON_X,
};

/// @brief  Switch Descriptor
static const tusb_desc_device_t nsDescriptor = {
    .bLength            = 18U,
    .bDescriptorType    = 1,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0x0f0d,
    .idProduct          = 0x0092,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01,
};

/// @brief PC string Descriptor
static char* hid_string_descriptor[5] = {
    // array of pointer to string descriptors
    (char[]){0x09, 0x04},   // 0: is supported language is English (0x0409)
    "Magfest",              // 1: Manufacturer
    "Swadge Controller",    // 2: Product
    "123456",               // 3: Serials, should use chip ID
    "Swadge HID interface", // 4: HID
};

/// @brief PC report Descriptor
static const uint8_t hid_report_descriptor[] = {TUD_HID_REPORT_DESC_GAMEPAD()};

/// @brief PC Config Descriptor
static const uint8_t hid_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1,                                                        // Configuration number
                          1,                                                        // interface count
                          0,                                                        // string index
                          (TUD_CONFIG_DESC_LEN + (CFG_TUD_HID * TUD_HID_DESC_LEN)), // total length
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,                       // attribute
                          100),                                                     // power in mA

    TUD_HID_DESCRIPTOR(0,                             // Interface number
                       4,                             // string index
                       false,                         // boot protocol
                       sizeof(hid_report_descriptor), // report descriptor len
                       0x81,                          // EP In address
                       16,                            // size
                       10),                           // polling interval
};

/// @brief PC tusb configuration
static const tinyusb_config_t pc_tusb_cfg = {
    .device_descriptor        = NULL,
    .string_descriptor        = hid_string_descriptor,
    .external_phy             = false,
    .configuration_descriptor = hid_configuration_descriptor,
};

// @brief Switch report descriptor by touchgadget
uint8_t const switch_hid_report_descriptor[] = {
    // Gamepad for Nintendo Switch
    // 14 buttons, 1 8-way dpad, 2 analog sticks (4 axes)
    0x05, 0x01,       // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,       // Usage (Game Pad)
    0xA1, 0x01,       // Collection (Application)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x35, 0x00,       //   Physical Minimum (0)
    0x45, 0x01,       //   Physical Maximum (1)
    0x75, 0x01,       //   Report Size (1)
    0x95, 0x0E,       //   Report Count (14)
    0x05, 0x09,       //   Usage Page (Button)
    0x19, 0x01,       //   Usage Minimum (0x01)
    0x29, 0x0E,       //   Usage Maximum (0x0E)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x02,       //   Report Count (2)
    0x81, 0x01,       //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,       //   Usage Page (Generic Desktop Ctrls)
    0x25, 0x07,       //   Logical Maximum (7)
    0x46, 0x3B, 0x01, //   Physical Maximum (315)
    0x75, 0x04,       //   Report Size (4)
    0x95, 0x01,       //   Report Count (1)
    0x65, 0x14,       //   Unit (System: English Rotation, Length: Centimeter)
    0x09, 0x39,       //   Usage (Hat switch)
    0x81, 0x42,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x65, 0x00,       //   Unit (None)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x01,       //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x46, 0xFF, 0x00, //   Physical Maximum (255)
    0x09, 0x30,       //   Usage (X)
    0x09, 0x31,       //   Usage (Y)
    0x09, 0x32,       //   Usage (Z)
    0x09, 0x35,       //   Usage (Rz)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x04,       //   Report Count (4)
    0x81, 0x02,       //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x01,       //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,             // End Collection
};

// @brief  Switch tusb configuration
static const tinyusb_config_t ns_tusb_cfg = {
    .device_descriptor        = &nsDescriptor,
    .string_descriptor        = NULL,
    .external_phy             = false,
    .configuration_descriptor = hid_configuration_descriptor,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * Enter the gamepad mode, allocate memory, initialize USB
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
    addSingleItemToMenu(gamepad->menu, str_pc);
    addSingleItemToMenu(gamepad->menu, str_ns);
    addSettingsOptionsItemToMenu(gamepad->menu, str_accel, accelSettingsOptions, accelSettingsValues,
                                 ARRAY_SIZE(accelSettingsOptions), getGamepadAccelSettingBounds(),
                                 getGamepadAccelSetting());
    addSingleItemToMenu(gamepad->menu, str_exit);

    // Initialize menu renderer
    gamepad->renderer = initMenuLogbookRenderer(&gamepad->logbookFont);

    // We shold go as fast as we can
    setFrameRateUs(0);

    // Set up the IMU
    accelSetRegistersAndReset();
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

void gamepadMainMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
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
    else
    {
        if (label == str_accel)
        {
            setGamepadAccelSetting(settingVal);
        }
    }
}

void gamepadStart(gamepadType_t type)
{
    gamepad->gamepadType = type;

    if (gamepad->gamepadType == GAMEPAD_NS)
    {
        initTusb(&ns_tusb_cfg, switch_hid_report_descriptor);
    }
    else
    {
        const uint8_t macBytes = 6; // This is part of the ESP API's design, and cannot be changed here
        uint8_t mac[macBytes];
        if (ESP_OK == esp_wifi_get_mac(WIFI_IF_STA, mac))
        {
            memcpy(&hid_string_descriptor[3], mac, macBytes);
        }
        initTusb(&pc_tusb_cfg, hid_report_descriptor);
    }

    gamepad->gpNsState.x  = 128;
    gamepad->gpNsState.y  = 128;
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

    switch (gamepad->screen)
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
            // No wifi mode stuff
    }

    accelIntegrate();
}

/**
 * Draw the gamepad state to the display when it changes
 *
 * @param elapsedUs unused
 */
void gamepadMainLoop(int64_t elapsedUs __attribute__((unused)))
{
    // Check if plugged in or not
    if (tud_ready() != gamepad->isPluggedIn)
    {
        gamepad->isPluggedIn = tud_ready();
    }

    // Clear the display
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c213);

    if (gamepad->exitTimer > 0)
    {
        gamepad->exitTimer += elapsedUs;
        int16_t numPx = (gamepad->exitTimer * TFT_WIDTH) / EXIT_TIME_US;
        fillDisplayArea(0, TFT_HEIGHT - 10, numPx, TFT_HEIGHT, c333);

        if (gamepad->exitTimer > EXIT_TIME_US)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }

    // Always Draw some reminder text, centered
    const char reminderText[] = "Menu + Pause to Exit";
    int16_t tWidth            = textWidth(&gamepad->ibmFont, reminderText);
    drawText(&gamepad->ibmFont, c555, reminderText, (TFT_WIDTH - tWidth) / 2, 10);

    if (gamepad->gamepadType == GAMEPAD_NS)
    {
        // Draw button combo text, centered
        const char captureText[] = "Down + Select:  Capture";
        tWidth                   = textWidth(&gamepad->ibmFont, captureText);
        int16_t textX            = (TFT_WIDTH - tWidth) / 2;
        int16_t afterText
            = drawText(&gamepad->ibmFont, c555, captureText, textX, TFT_HEIGHT - gamepad->ibmFont.height * 2 - 12);

        const char homeText1[] = "Down + Start:";
        drawText(&gamepad->ibmFont, c555, homeText1, textX, TFT_HEIGHT - gamepad->ibmFont.height - 10);

        const char* homeText2 = getButtonName(GAMEPAD_NS_BUTTON_HOME);
        tWidth                = textWidth(&gamepad->ibmFont, homeText2);
        drawText(&gamepad->ibmFont, c555, homeText2, afterText - tWidth - 1, TFT_HEIGHT - gamepad->ibmFont.height - 10);
    }

    // If it's plugged in, draw buttons
    if (gamepad->isPluggedIn)
    {
        // Helper function pointer
        void (*drawFunc)(int, int, int, paletteColor_t);

        // A list of all the hat directions, in order
        static const uint8_t hatDirs[] = {
            GAMEPAD_HAT_UP,   GAMEPAD_HAT_UP_RIGHT,  GAMEPAD_HAT_RIGHT, GAMEPAD_HAT_DOWN_RIGHT,
            GAMEPAD_HAT_DOWN, GAMEPAD_HAT_DOWN_LEFT, GAMEPAD_HAT_LEFT,  GAMEPAD_HAT_UP_LEFT,
        };

        // For each hat direction
        for (uint8_t i = 0; i < ARRAY_SIZE(hatDirs); i++)
        {
            // The degree around the cluster
            int16_t deg = i * 45;
            // The center of the cluster
            int16_t xc = TFT_WIDTH / 4;
            int16_t yc = (TFT_HEIGHT / 2) - DPAD_CLUSTER_Y_OFF + Y_OFF;
            // Draw the button around the cluster
            xc += ((getSin1024(deg) * DPAD_CLUSTER_RADIUS) / 1024);
            yc += ((-getCos1024(deg) * DPAD_CLUSTER_RADIUS) / 1024);

            // Draw either a filled or outline circle, if this is the direction pressed
            switch (gamepad->gamepadType)
            {
                case GAMEPAD_NS:
                {
                    drawFunc = (gamepad->gpNsState.hat == (hatDirs[i] - 1)) ? &drawCircleFilled : &drawCircle;
                    break;
                }
                case GAMEPAD_GENERIC:
                default:
                {
                    drawFunc = (gamepad->gpState.hat == hatDirs[i]) ? &drawCircleFilled : &drawCircle;
                    break;
                }
            }

            drawFunc(xc, yc, DPAD_BTN_RADIUS, c551 /*paletteHsvToHex(i * 32, 0xFF, 0xFF)*/);
        }

        // Select button
        switch (gamepad->gamepadType)
        {
            case GAMEPAD_NS:
            {
                drawFunc = (gamepad->gpNsState.buttons & GAMEPAD_NS_BUTTON_MINUS) ? &drawCircleFilled : &drawCircle;
                break;
            }
            case GAMEPAD_GENERIC:
            default:
            {
                drawFunc = (gamepad->gpState.buttons & GAMEPAD_BUTTON_SELECT) ? &drawCircleFilled : &drawCircle;
                break;
            }
        }
        int16_t x = (TFT_WIDTH / 2) - START_BTN_RADIUS - START_BTN_SEP;
        int16_t y = ((3 * TFT_WIDTH) / 4) - START_BTN_Y_OFF + Y_OFF;
        drawFunc(x, y, START_BTN_RADIUS, c333);

        if (gamepad->gamepadType == GAMEPAD_NS)
        {
            const char* buttonName = getButtonName(GAMEPAD_NS_BUTTON_MINUS);
            drawText(&gamepad->ibmFont, c444, buttonName, x - textWidth(&gamepad->ibmFont, buttonName) / 2,
                     y - gamepad->ibmFont.height / 2);
        }

        // Start button
        switch (gamepad->gamepadType)
        {
            case GAMEPAD_NS:
            {
                drawFunc = (gamepad->gpNsState.buttons & GAMEPAD_NS_BUTTON_PLUS) ? &drawCircleFilled : &drawCircle;
                break;
            }
            case GAMEPAD_GENERIC:
            default:
            {
                drawFunc = (gamepad->gpState.buttons & GAMEPAD_BUTTON_START) ? &drawCircleFilled : &drawCircle;
                break;
            }
        }
        x = (TFT_WIDTH / 2) + START_BTN_RADIUS + START_BTN_SEP;
        drawFunc(x, y, START_BTN_RADIUS, c333);

        if (gamepad->gamepadType == GAMEPAD_NS)
        {
            const char* buttonName = getButtonName(GAMEPAD_NS_BUTTON_PLUS);
            drawText(&gamepad->ibmFont, c444, buttonName, x - textWidth(&gamepad->ibmFont, buttonName) / 2,
                     y - gamepad->ibmFont.height / 2);
        }

        // Button A
        switch (gamepad->gamepadType)
        {
            case GAMEPAD_NS:
            {
                drawFunc = (gamepad->gpNsState.buttons & GAMEPAD_NS_BUTTON_A) ? &drawCircleFilled : &drawCircle;
                break;
            }
            case GAMEPAD_GENERIC:
            default:
            {
                drawFunc = (gamepad->gpState.buttons & GAMEPAD_BUTTON_A) ? &drawCircleFilled : &drawCircle;
                break;
            }
        }
        drawFunc(((3 * TFT_WIDTH) / 4) + AB_BTN_RADIUS + AB_BTN_SEP,
                 (TFT_HEIGHT / 4) - AB_BTN_Y_SEP - AB_BTN_Y_OFF + Y_OFF, AB_BTN_RADIUS, c243);

        // Button B
        switch (gamepad->gamepadType)
        {
            case GAMEPAD_NS:
            {
                drawFunc = (gamepad->gpNsState.buttons & GAMEPAD_NS_BUTTON_B) ? &drawCircleFilled : &drawCircle;
                break;
            }
            case GAMEPAD_GENERIC:
            default:
            {
                drawFunc = (gamepad->gpState.buttons & GAMEPAD_BUTTON_B) ? &drawCircleFilled : &drawCircle;
                break;
            }
        }
        drawFunc(((3 * TFT_WIDTH) / 4) - AB_BTN_RADIUS - AB_BTN_SEP,
                 (TFT_HEIGHT / 4) + AB_BTN_Y_SEP - AB_BTN_Y_OFF + Y_OFF, AB_BTN_RADIUS, c401);

        // Draw touch pad
        int16_t tBarX = TFT_WIDTH - TOUCHPAD_DIAM / 2 - TOUCHPAD_X_OFF;

        bool touched;
        int32_t phi, r, intensity;
        touched = getTouchJoystick(&phi, &r, &intensity);

        if (!touched)
        {
            phi       = 0;
            r         = 0;
            intensity = 0;
        }

        touchDrawVector(&gamepad->ibmFont, "", c333, tBarX, TFT_HEIGHT / 2 - TOUCHPAD_Y_OFF + Y_OFF, TOUCHPAD_DIAM / 2,
                        touched, phi, r);

        if (getGamepadAccelSetting() && gamepad->gamepadType == GAMEPAD_GENERIC)
        {
            // Declare variables to receive acceleration
            int16_t a_x, a_y, a_z;
            // Get the current acceleration
            if (ESP_OK == accelIntegrate() && ESP_OK == accelGetOrientVec(&a_x, &a_y, &a_z))
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
        switch (gamepad->gamepadType)
        {
            case GAMEPAD_NS:
            {
                plugInText = "Plug USB-C into Switch please!";
                break;
            }
            case GAMEPAD_GENERIC:
            default:
            {
                plugInText = "Plug USB-C into computer please!";
                break;
            }
        }
        tWidth = textWidth(&gamepad->ibmFont, plugInText);
        drawText(&gamepad->ibmFont, c555, plugInText, (TFT_WIDTH - tWidth) / 2,
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
    switch (gamepad->gamepadType)
    {
        case GAMEPAD_GENERIC:
        {
            // Build a list of all independent buttons held down
            gamepad->gpState.buttons
                &= ~(GAMEPAD_BUTTON_A | GAMEPAD_BUTTON_B | GAMEPAD_BUTTON_START | GAMEPAD_BUTTON_SELECT);
            if (evt->state & PB_A)
            {
                gamepad->gpState.buttons |= GAMEPAD_BUTTON_A;
            }
            if (evt->state & PB_B)
            {
                gamepad->gpState.buttons |= GAMEPAD_BUTTON_B;
            }
            if (evt->state & PB_START)
            {
                gamepad->gpState.buttons |= GAMEPAD_BUTTON_START;
            }
            if (evt->state & PB_SELECT)
            {
                gamepad->gpState.buttons |= GAMEPAD_BUTTON_SELECT;
            }

            // Figure out which way the D-Pad is pointing
            gamepad->gpState.hat = GAMEPAD_HAT_CENTERED;
            if (evt->state & PB_UP)
            {
                if (evt->state & PB_RIGHT)
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_UP_RIGHT;
                }
                else if (evt->state & PB_LEFT)
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_UP_LEFT;
                }
                else
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_UP;
                }
            }
            else if (evt->state & PB_DOWN)
            {
                if (evt->state & PB_RIGHT)
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_DOWN_RIGHT;
                }
                else if (evt->state & PB_LEFT)
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_DOWN_LEFT;
                }
                else
                {
                    gamepad->gpState.hat = GAMEPAD_HAT_DOWN;
                }
            }
            else if (evt->state & PB_RIGHT)
            {
                gamepad->gpState.hat = GAMEPAD_HAT_RIGHT;
            }
            else if (evt->state & PB_LEFT)
            {
                gamepad->gpState.hat = GAMEPAD_HAT_LEFT;
            }

            if (evt->button == PB_START || evt->button == PB_SELECT)
            {
                if ((evt->state & PB_START) && (evt->state & PB_SELECT))
                {
                    gamepad->exitTimer = 1;
                }
                else
                {
                    gamepad->exitTimer = 0;
                }
            }

            break;
        }
        case GAMEPAD_NS:
        {
            // Build a list of all independent buttons held down
            gamepad->gpNsState.buttons
                &= ~(GAMEPAD_NS_BUTTON_A | GAMEPAD_NS_BUTTON_B | GAMEPAD_NS_BUTTON_PLUS | GAMEPAD_NS_BUTTON_MINUS
                     | GAMEPAD_NS_BUTTON_HOME | GAMEPAD_NS_BUTTON_CAPTURE);

            if (evt->state & PB_A)
            {
                gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_A;
            }
            if (evt->state & PB_B)
            {
                gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_B;
            }
            if (evt->state & PB_START)
            {
                if (evt->state & PB_DOWN)
                {
                    gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_HOME;
                }
                else
                {
                    gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_PLUS;
                }
            }
            if (evt->state & PB_SELECT)
            {
                if (evt->state & PB_DOWN)
                {
                    gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_CAPTURE;
                }
                else
                {
                    gamepad->gpNsState.buttons |= GAMEPAD_NS_BUTTON_MINUS;
                }
            }

            // Figure out which way the D-Pad is pointing
            gamepad->gpNsState.hat = GAMEPAD_NS_HAT_CENTERED;
            if (evt->state & PB_UP)
            {
                if (evt->state & PB_RIGHT)
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_UP_RIGHT;
                }
                else if (evt->state & PB_LEFT)
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_UP_LEFT;
                }
                else
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_UP;
                }
            }
            else if (evt->state & PB_DOWN)
            {
                if (evt->state & PB_RIGHT)
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_DOWN_RIGHT;
                }
                else if (evt->state & PB_LEFT)
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_DOWN_LEFT;
                }
                else
                {
                    gamepad->gpNsState.hat = GAMEPAD_NS_HAT_DOWN;
                }
            }
            else if (evt->state & PB_RIGHT)
            {
                gamepad->gpNsState.hat = GAMEPAD_NS_HAT_RIGHT;
            }
            else if (evt->state & PB_LEFT)
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
    if (tud_ready())
    {
        switch (gamepad->gamepadType)
        {
            case GAMEPAD_GENERIC:
            {
                bool touched;
                int32_t phi, r, intensity;
                touched = getTouchJoystick(&phi, &r, &intensity);
                if (touched)
                {
                    int32_t x, y;
                    getTouchCartesian(phi, r, &x, &y);
                    gamepad->gpState.x = (255 * x) / 1024 - 128;
                    gamepad->gpState.y = (-255 * y) / 1024 - 128;
                    // gamepad->gpState.z = (127 * (phi - 180)) / 360;
                    gamepad->gpState.z = 0;
                }
                else
                {
                    gamepad->gpState.x = 0;
                    gamepad->gpState.y = 0;
                    gamepad->gpState.z = 0;
                }

                // Send the state over USB
                tud_hid_gamepad_report(HID_ITF_PROTOCOL_NONE, gamepad->gpState.x, gamepad->gpState.y,
                                       gamepad->gpState.z, gamepad->gpState.rx, gamepad->gpState.ry,
                                       gamepad->gpState.rz, gamepad->gpState.hat, gamepad->gpState.buttons);

                break;
            }
            case GAMEPAD_NS:
            {
                // TODO: accel
                // TODO check this
                // tud_gamepad_ns_report(&gamepad->gpNsState);
                tud_hid_gamepad_report_ns(HID_ITF_PROTOCOL_NONE, gamepad->gpNsState.x, gamepad->gpNsState.y,
                                          gamepad->gpNsState.z, gamepad->gpNsState.rz, gamepad->gpNsState.rx,
                                          gamepad->gpNsState.ry, gamepad->gpNsState.hat, gamepad->gpNsState.buttons);

                break;
            }
        }
    }
}

static const char* getButtonName(hid_gamepad_button_bm_t button)
{
    switch (button)
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
