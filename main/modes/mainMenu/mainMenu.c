//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"

#include "mainMenu.h"
#include "demoMode.h"
#include "jukebox.h"
#include "pushy.h"
#include "pong.h"
#include "marbles.h"
#include "mode_paint.h"
#include "colorchord.h"
#include "lumberjack.h"
#include "dance.h"
#include "tunernome.h"
#include "touchTest.h"

#include "settingsManager.h"
#include "breakout.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;
    menuLogbookRenderer_t* renderer;
    font_t logbook;
    song_t jingle;
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
static const char mainMenuName[] = "MAIN MENU";

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

static const char tftSettingLabel[]          = "TFT";
static const char ledSettingLabel[]          = "LED";
static const char bgmVolSettingLabel[]       = "BGM";
static const char sfxVolSettingLabel[]       = "SFX";
static const char micSettingLabel[]          = "MIC";
static const char screenSaverSettingsLabel[] = "Screensaver: ";

static const int32_t screenSaverSettingsValues[] = {
    0,   // Off
    10,  // 10sec
    20,  // 20sec
    30,  // 30sec
    60,  // 60sec
    120, // 2min
    300, // 5min
};

static const char* const screenSaverSettingsOptions[] = {
    "Off", "10s", "20s", "30s", "1m", "2m", "5m",
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the main menu mode
 */
static void mainMenuEnterMode(void)
{
    // Allocate memory for the mode
    mainMenu = calloc(1, sizeof(mainMenu_t));

    // Load a font
    loadFont("logbook.font", &mainMenu->logbook, false);

    // Load a song for when the volume changes
    loadSong("jingle.sng", &mainMenu->jingle, false);

    // Allocate the menu
    mainMenu->menu = initMenu(mainMenuName, mainMenuCb);

    // Add single items
    addSingleItemToMenu(mainMenu->menu, lumberjackMode.modeName);
    addSingleItemToMenu(mainMenu->menu, pongMode.modeName);
    addSingleItemToMenu(mainMenu->menu, marblesMode.modeName);
    addSingleItemToMenu(mainMenu->menu, colorchordMode.modeName);
    addSingleItemToMenu(mainMenu->menu, modePaint.modeName);
    addSingleItemToMenu(mainMenu->menu, breakoutMode.modeName);
    addSingleItemToMenu(mainMenu->menu, danceMode.modeName);
    addSingleItemToMenu(mainMenu->menu, pushyMode.modeName);
    addSingleItemToMenu(mainMenu->menu, tunernomeMode.modeName);
    addSingleItemToMenu(mainMenu->menu, jukeboxMode.modeName);
    addSingleItemToMenu(mainMenu->menu, touchTestMode.modeName);

    // Start a submenu for settings
    mainMenu->menu = startSubMenu(mainMenu->menu, settingsLabel);
    // Get the bounds and current settings to build this menu
    addSettingsItemToMenu(mainMenu->menu, tftSettingLabel, getTftBrightnessSettingBounds(), getTftBrightnessSetting());
    addSettingsItemToMenu(mainMenu->menu, ledSettingLabel, getLedBrightnessSettingBounds(), getLedBrightnessSetting());
    addSettingsItemToMenu(mainMenu->menu, bgmVolSettingLabel, getBgmVolumeSettingBounds(), getBgmVolumeSetting());
    addSettingsItemToMenu(mainMenu->menu, sfxVolSettingLabel, getSfxVolumeSettingBounds(), getSfxVolumeSetting());
    addSettingsItemToMenu(mainMenu->menu, micSettingLabel, getMicGainSettingBounds(), getMicGainSetting());

    addSettingsOptionsItemToMenu(mainMenu->menu, screenSaverSettingsLabel, screenSaverSettingsOptions,
                                 screenSaverSettingsValues, ARRAY_SIZE(screenSaverSettingsValues),
                                 getScreensaverTimeSettingBounds(), getScreensaverTimeSetting());
    // End the submenu for settings
    mainMenu->menu = endSubMenu(mainMenu->menu);

    // Initialize menu renderer
    mainMenu->renderer = initMenuLogbookRenderer(&mainMenu->logbook);
}

/**
 * @brief Deinitialize the main menu mode
 */
static void mainMenuExitMode(void)
{
    // Deinit menu
    deinitMenu(mainMenu->menu);

    // Deinit renderer
    deinitMenuLogbookRenderer(mainMenu->renderer);

    // Free the font
    freeFont(&mainMenu->logbook);

    // Free the song
    freeSong(&mainMenu->jingle);

    // Free mode memory
    free(mainMenu);
}

/**
 * @brief Main loop for the main menu. Simply handles menu input and draws the menu
 *
 * @param elapsedUs unused
 */
static void mainMenuMainLoop(int64_t elapsedUs)
{
    // Pass all button events to the menu
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        mainMenu->menu = menuButton(mainMenu->menu, evt);
    }

    // Draw the menu
    drawMenuLogbook(mainMenu->menu, mainMenu->renderer, elapsedUs);
}

/**
 * @brief Callback for when menu items are selected
 *
 * @param label The menu item that was selected or moved to
 * @param selected true if the item was selected, false if it was moved to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void mainMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    // Stop the buzzer first no matter what, so that it turns off
    // if we scroll away from the BGM or SFX settings.
    bzrStop();

    if (selected)
    {
        // These items enter other modes, so they must be selected
        if (label == demoMode.modeName)
        {
            switchToSwadgeMode(&demoMode);
        }
        else if (label == pongMode.modeName)
        {
            switchToSwadgeMode(&pongMode);
        }
        else if (label == marblesMode.modeName)
        {
            switchToSwadgeMode(&marblesMode);
        }
        else if (label == modePaint.modeName)
        {
            switchToSwadgeMode(&modePaint);
        }
        else if (label == colorchordMode.modeName)
        {
            switchToSwadgeMode(&colorchordMode);
        }
        else if (label == breakoutMode.modeName)
        {
            switchToSwadgeMode(&breakoutMode);
        }
        else if (label == danceMode.modeName)
        {
            switchToSwadgeMode(&danceMode);
        }
        else if (label == pushyMode.modeName)
        {
            switchToSwadgeMode(&pushyMode);
        }
        else if (label == tunernomeMode.modeName)
        {
            switchToSwadgeMode(&tunernomeMode);
        }
        else if (label == jukeboxMode.modeName)
        {
            switchToSwadgeMode(&jukeboxMode);
        }
        else if (label == touchTestMode.modeName)
        {
            switchToSwadgeMode(&touchTestMode);
        }
        else if (label == lumberjackMode.modeName)
        {
            switchToSwadgeMode(&lumberjackMode);
        }
    }
    else
    {
        // Settings are not selected, they're scrolled to
        if (tftSettingLabel == label)
        {
            setTftBrightnessSetting(settingVal);
        }
        else if (ledSettingLabel == label)
        {
            setLedBrightnessSetting(settingVal);
        }
        else if (bgmVolSettingLabel == label)
        {
            setBgmVolumeSetting(settingVal);
            bzrPlayBgm(&mainMenu->jingle, BZR_STEREO);
        }
        else if (sfxVolSettingLabel == label)
        {
            setSfxVolumeSetting(settingVal);
            bzrPlaySfx(&mainMenu->jingle, BZR_STEREO);
        }
        else if (micSettingLabel == label)
        {
            setMicGainSetting(settingVal);
        }
        else if (screenSaverSettingsLabel == label)
        {
            setScreensaverTimeSetting(settingVal);
        }
    }
}
