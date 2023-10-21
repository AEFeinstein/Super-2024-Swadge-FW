//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include "tinyusb.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "touchUtils.h"
#include "gamepad.h"
#include "mainMenu.h"

//==============================================================================
// Defines
//==============================================================================

#define Y_OFF 20

#define DPAD_BTN_RADIUS     16
#define DPAD_CLUSTER_RADIUS 45

#define START_BTN_RADIUS 10
#define START_BTN_SEP    2

#define AB_BTN_RADIUS 25
#define AB_BTN_Y_OFF  8
#define AB_BTN_SEP    2

#define ACCEL_BAR_HEIGHT 8
#define ACCEL_BAR_SEP    1
#define MAX_ACCEL_BAR_W  100

#define TOUCHBAR_WIDTH  100
#define TOUCHBAR_HEIGHT 20
#define TOUCHBAR_Y_OFF  55
// #define TOUCHBAR_ANALOG_HEIGHT 8

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

/// Switch Gamepad Buttons Bitmap
typedef enum
{
    GAMEPAD_NS_BUTTON_Y       = 0x01,
    GAMEPAD_NS_BUTTON_B       = 0x02,
    GAMEPAD_NS_BUTTON_A       = 0x04,
    GAMEPAD_NS_BUTTON_X       = 0x08,
    GAMEPAD_NS_BUTTON_TL      = 0x10,
    GAMEPAD_NS_BUTTON_TR      = 0x20,
    GAMEPAD_NS_BUTTON_TL2     = 0x40,
    GAMEPAD_NS_BUTTON_TR2     = 0x80,
    GAMEPAD_NS_BUTTON_MINUS   = 0x100,
    GAMEPAD_NS_BUTTON_PLUS    = 0x200,
    GAMEPAD_NS_BUTTON_THUMBL  = 0x400,
    GAMEPAD_NS_BUTTON_THUMBR  = 0x800,
    GAMEPAD_NS_BUTTON_HOME    = 0x1000,
    GAMEPAD_NS_BUTTON_CAPTURE = 0x2000,
    GAMEPAD_NS_BUTTON_Z       = 0x4000, /// UNUSED?
} hid_gamepad_ns_button_bm_t;

/// Switch Gamepad HAT/DPAD Buttons (from Linux input event codes)
typedef enum
{
    GAMEPAD_NS_HAT_CENTERED   = 8, ///< DPAD_CENTERED
    GAMEPAD_NS_HAT_UP         = 0, ///< DPAD_UP
    GAMEPAD_NS_HAT_UP_RIGHT   = 1, ///< DPAD_UP_RIGHT
    GAMEPAD_NS_HAT_RIGHT      = 2, ///< DPAD_RIGHT
    GAMEPAD_NS_HAT_DOWN_RIGHT = 3, ///< DPAD_DOWN_RIGHT
    GAMEPAD_NS_HAT_DOWN       = 4, ///< DPAD_DOWN
    GAMEPAD_NS_HAT_DOWN_LEFT  = 5, ///< DPAD_DOWN_LEFT
    GAMEPAD_NS_HAT_LEFT       = 6, ///< DPAD_LEFT
    GAMEPAD_NS_HAT_UP_LEFT    = 7, ///< DPAD_UP_LEFT
} hid_gamepad_ns_hat_t;

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

void gamepadMainMenuCb(const char* label, bool selected, uint32_t settingVal);
void gamepadMenuLoop(int64_t elapsedUs);
void gamepadStart(gamepadType_t type);

static bool saveGamepadToggleSettings(union gamepadToggleSettings_u* toggleSettings);
static bool loadGamepadToggleSettings(union gamepadToggleSettings_u* toggleSettings);

static const char* getButtonName(hid_gamepad_button_bm_t button);

//==============================================================================
// Variables
//==============================================================================

static const char str_pc[]               = "PC";
static const char str_ns[]               = "Switch";
static const char str_touch_analog_on[]  = "Touch: Digi+Analog";
static const char str_touch_analog_off[] = "Touch: Digital Only";
static const char str_accel_on[]         = "Accel: On";
static const char str_accel_off[]        = "Accel: Off";
static const char str_exit[]             = "Exit";
static const char KEY_GAMEPAD_SETTINGS[] = "gp_settings";

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

// TODO: handle 8-way joystick
const hid_gamepad_button_bm_t touchMap[] = {
    GAMEPAD_BUTTON_C, GAMEPAD_BUTTON_X, GAMEPAD_BUTTON_Y, GAMEPAD_BUTTON_Z, GAMEPAD_BUTTON_TL,
};

// TODO: handle 8-way joystick?
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
static const char* hid_string_descriptor[5] = {
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
    addSingleItemToMenu(gamepad->menu, gamepad->gamepadToggleSettings.settings.touchAnalogOn ? str_touch_analog_on
                                                                                             : str_touch_analog_off);
    addSingleItemToMenu(gamepad->menu, gamepad->gamepadToggleSettings.settings.accelOn ? str_accel_on : str_accel_off);
    addSingleItemToMenu(gamepad->menu, str_exit);

    loadGamepadToggleSettings(&gamepad->gamepadToggleSettings);

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
    else // TODO: verify this is the proper way to use `selected`
    {
        bool needRedraw = false;

        if (label == str_touch_analog_on)
        {
            // Touch analog is on, turn it off
            gamepad->gamepadToggleSettings.settings.touchAnalogOn = false;
            saveGamepadToggleSettings(&gamepad->gamepadToggleSettings);

            needRedraw = true;
        }
        else if (label == str_touch_analog_off)
        {
            // Touch analog is off, turn it on
            gamepad->gamepadToggleSettings.settings.touchAnalogOn = true;
            saveGamepadToggleSettings(&gamepad->gamepadToggleSettings);

            needRedraw = true;
        }
        else if (label == str_accel_on)
        {
            // Accel is on, turn it off
            gamepad->gamepadToggleSettings.settings.accelOn = false;
            saveGamepadToggleSettings(&gamepad->gamepadToggleSettings);

            needRedraw = true;
        }
        else if (label == str_accel_off)
        {
            // Accel is off, turn it on
            gamepad->gamepadToggleSettings.settings.accelOn = true;
            saveGamepadToggleSettings(&gamepad->gamepadToggleSettings);

            needRedraw = true;
        }

        if (needRedraw)
        {
            gamepad->screen = GAMEPAD_MENU;
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

    // Always Draw some reminder text, centered
    const char reminderText[] = "Start + Select to Exit";
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
            int16_t yc = (TFT_HEIGHT / 2) + Y_OFF;
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
        int16_t y = (TFT_HEIGHT / 4) + Y_OFF;
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
        drawFunc(((3 * TFT_WIDTH) / 4) + AB_BTN_RADIUS + AB_BTN_SEP, (TFT_HEIGHT / 2) - AB_BTN_Y_OFF + Y_OFF,
                 AB_BTN_RADIUS, c243);

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
        drawFunc(((3 * TFT_WIDTH) / 4) - AB_BTN_RADIUS - AB_BTN_SEP, (TFT_HEIGHT / 2) + AB_BTN_Y_OFF + Y_OFF,
                 AB_BTN_RADIUS, c401);

        // Draw touch strip
        int16_t tBarX = TFT_WIDTH - TOUCHBAR_WIDTH;

        // TODO: translate new touchpad data into 4-way or 8-way joystick
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
        if (gamepad->gamepadType == GAMEPAD_GENERIC && gamepad->gamepadToggleSettings.settings.touchAnalogOn)
        {
            int32_t phi, r, intensity;
            if (getTouchJoystick(&phi, &r, &intensity))
            {
                // TODO: rebuild this for new touchpad
            }

            // drawRect(
            //          tBarX - 1       , TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT                          - 1,
            //          TFT_WIDTH, TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT + TOUCHBAR_ANALOG_HEIGHT + 1,
            //          c111);
            // fillDisplayArea(
            //                 tBarX + center - 1, TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT,
            //                 tBarX + center + 1, TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT + TOUCHBAR_ANALOG_HEIGHT,
            //                 c444);
        }

        uint8_t numTouchElem = ARRAY_SIZE(touchMap);
        for (uint8_t touchIdx = 0; touchIdx < numTouchElem; touchIdx++)
        {
            int16_t x1 = tBarX - 1;
            int16_t x2 = tBarX + (TOUCHBAR_WIDTH / numTouchElem);

            if ((gamepad->gamepadType == GAMEPAD_GENERIC) ? gamepad->gpState.buttons & touchMap[touchIdx]
                                                          : gamepad->gpNsState.buttons & touchMapNs[touchIdx])
            {
                fillDisplayArea(x1, TOUCHBAR_Y_OFF, x2, TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT, c111);
            }
            else
            {
                drawRect(x1, TOUCHBAR_Y_OFF, x2, TOUCHBAR_Y_OFF + TOUCHBAR_HEIGHT, c111);
            }

            if (gamepad->gamepadType == GAMEPAD_NS)
            {
                const char* buttonName = getButtonName(touchMapNs[touchIdx]);
                drawText(&gamepad->ibmFont, c444, buttonName,
                         x1 + (x2 - x1 - textWidth(&gamepad->ibmFont, buttonName)) / 2,
                         TOUCHBAR_Y_OFF + (TOUCHBAR_HEIGHT - gamepad->ibmFont.height) / 2);
            }

            tBarX += (TOUCHBAR_WIDTH / numTouchElem);
        }

        if (gamepad->gamepadToggleSettings.settings.accelOn && gamepad->gamepadType == GAMEPAD_GENERIC)
        {
            // Declare variables to receive acceleration
            int16_t a_x, a_y, a_z;
            // Get the current acceleration
            if (ESP_OK == accelGetOrientVec(&a_x, &a_y, &a_z))
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
                // TODO: handle 8-way joystick
                if (gamepad->gamepadToggleSettings.settings.touchAnalogOn
                    && gamepad->gpState.buttons
                           & ((touchMap[0] | touchMap[1] | touchMap[2]
                               | touchMap[3]))) // | touchMap[4] | touchMap[5] | touchMap[6] | touchMap[7])))
                {
                    int32_t phi, r, intensity;
                    if (getTouchJoystick(&phi, &r, &intensity))
                    {
                        int32_t x, y;
                        getTouchCartesian(phi, r, &x, &y);
                        gamepad->gpState.z = (127 * (phi - 180)) / 360;
                    }
                    else
                    {
                        gamepad->gpState.z = 0;
                    }
                }
                else
                {
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
        toggleSettings->settings.accelOn       = true;
        toggleSettings->settings.touchAnalogOn = true;
        return saveGamepadToggleSettings(toggleSettings);
    }
    return true;
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
