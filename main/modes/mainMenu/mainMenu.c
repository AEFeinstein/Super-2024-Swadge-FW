//==============================================================================
// Includes
//==============================================================================

#include "mainMenu.h"
#include "menu.h"
#include "swadge2024.h"

#include "demoMode.h"
#include "pong.h"
#include "mode_colorchord.h"
#include "settingsManager.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;
    font_t ibm;
} mainMenu_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void mainMenuEnterMode(void);
static void mainMenuExitMode(void);
static void mainMenuMainLoop(int64_t elapsedUs);
static void mainMenuCb(const char* label, bool selected, uint32_t settingVal);

//==============================================================================
// Variables
//==============================================================================

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
static const char mainMenuName[] = "Main Menu";

swadgeMode_t mainMenuMode = {
    .modeName                 = mainMenuName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = true,
    .fnEnterMode              = mainMenuEnterMode,
    .fnExitMode               = mainMenuExitMode,
    .fnMainLoop               = mainMenuMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

mainMenu_t* mainMenu;

static const char settingsLabel[] = "Settings";

static const char tftSettingLabel[] = "TFT";
static const char ledSettingLabel[] = "LED";
static const char volSettingLabel[] = "VOL";

// Test tone when the volume changes
static musicalNote_t volTestToneNotes[] = {
    {
        .note   = G_4,
        .timeMs = 400,
    },
    {
        .note   = E_5,
        .timeMs = 400,
    },
    {
        .note   = C_5,
        .timeMs = 400,
    },
};
static const song_t volTestTone = {
    .loopStartNote = 0,
    .notes         = volTestToneNotes,
    .numNotes      = ARRAY_SIZE(volTestToneNotes),
    .shouldLoop    = false,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
static void mainMenuEnterMode(void)
{
    // Allocate memory for the mode
    mainMenu = calloc(1, sizeof(mainMenu_t));

    // Load a font
    loadFont("ibm_vga8.font", &mainMenu->ibm, false);

    // Allocate the menu
    mainMenu->menu = initMenu(mainMenuName, mainMenuCb);

    // Add single items
    addSingleItemToMenu(mainMenu->menu, demoMode.modeName);
    addSingleItemToMenu(mainMenu->menu, pongMode.modeName);
    addSingleItemToMenu(mainMenu->menu, colorchordMode.modeName);

    mainMenu->menu = startSubMenu(mainMenu->menu, settingsLabel);
    // TODO extract bounds and current val from settings
    addSettingsItemToMenu(mainMenu->menu, tftSettingLabel, 0, 8, 4);
    addSettingsItemToMenu(mainMenu->menu, ledSettingLabel, 0, 8, 4);
    addSettingsItemToMenu(mainMenu->menu, volSettingLabel, 0, 13, 4);
    mainMenu->menu = endSubMenu(mainMenu->menu);
}

/**
 * @brief TODO
 *
 */
static void mainMenuExitMode(void)
{
    // Deinit menu
    deinitMenu(mainMenu->menu);

    // Free the font
    freeFont(&mainMenu->ibm);

    // Free mode memory
    free(mainMenu);
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
static void mainMenuMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        mainMenu->menu = menuButton(mainMenu->menu, evt);
    }

    // Draw the menu
    drawMenu(mainMenu->menu, &mainMenu->ibm);

    // Set LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};

    leds[0].r = 0xFF;
    leds[1].g = 0xFF;
    leds[2].b = 0xFF;
    leds[3].r = 0xFF;
    leds[4].g = 0xFF;
    leds[5].b = 0xFF;
    leds[6].r = 0xFF;
    leds[7].g = 0xFF;
    setLeds(leds, 8);
}

/**
 * @brief TODO
 *
 * @param label
 * @param selected
 */
static void mainMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == demoMode.modeName)
        {
            switchToSwadgeMode(&demoMode);
        }
        else if (label == pongMode.modeName)
        {
            switchToSwadgeMode(&pongMode);
        }
        else if (label == colorchordMode.modeName)
        {
            switchToSwadgeMode(&colorchordMode);
        }
    }
    else
    {
        if (tftSettingLabel == label)
        {
            setTftBrightnessSetting(settingVal);
        }
        else if (ledSettingLabel == label)
        {
            setLedBrightnessSetting(settingVal);
        }
        else if (volSettingLabel == label)
        {
            setBgmVolumeSetting(settingVal);
            bzrPlayBgm(&volTestTone);
        }
    }
}
