//==============================================================================
// Includes
//==============================================================================

#include "mainMenu.h"
#include "menu.h"
#include "swadge2024.h"

#include "demoMode.h"
#include "pong.h"
#include "mode_colorchord.h"
#include "mode_shooter.h"
#include "mode_ray.h"
#include "settingsManager.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;
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

static const char tftSettingLabel[]         = "TFT";
static const char ledSettingLabel[]         = "LED";
static const char bgmVolSettingLabel[]      = "BGM";
static const char sfxVolSettingLabel[]      = "SFX";
static const char micSettingLabel[]         = "MIC";
static const char screenSaverSettingLabel[] = "Screen Saver";

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
    addSingleItemToMenu(mainMenu->menu, demoMode.modeName);
    addSingleItemToMenu(mainMenu->menu, pongMode.modeName);
    addSingleItemToMenu(mainMenu->menu, colorchordMode.modeName);
    addSingleItemToMenu(mainMenu->menu, shooterMode.modeName);
    addSingleItemToMenu(mainMenu->menu, rayMode.modeName);

    // Start a submenu for settings
    mainMenu->menu = startSubMenu(mainMenu->menu, settingsLabel);
    // Get the bounds and current settings to build this menu
    addSettingsItemToMenu(mainMenu->menu, tftSettingLabel, getTftBrightnessSettingBounds(), getTftBrightnessSetting());
    addSettingsItemToMenu(mainMenu->menu, ledSettingLabel, getLedBrightnessSettingBounds(), getLedBrightnessSetting());
    addSettingsItemToMenu(mainMenu->menu, bgmVolSettingLabel, getBgmVolumeSettingBounds(), getBgmVolumeSetting());
    addSettingsItemToMenu(mainMenu->menu, sfxVolSettingLabel, getSfxVolumeSettingBounds(), getSfxVolumeSetting());
    addSettingsItemToMenu(mainMenu->menu, micSettingLabel, getMicGainSettingBounds(), getMicGainSetting());
    addSettingsItemToMenu(mainMenu->menu, screenSaverSettingLabel, getScreensaverTimeSettingBounds(),
                          getScreensaverTimeSetting());
    // End the submenu for settings
    mainMenu->menu = endSubMenu(mainMenu->menu);
}

/**
 * @brief Deinitialize the main menu mode
 */
static void mainMenuExitMode(void)
{
    // Deinit menu
    deinitMenu(mainMenu->menu);

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
    drawMenu(mainMenu->menu, &mainMenu->logbook);

    // Set LEDs, just because
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
 * @brief Callback for when menu items are selected
 *
 * @param label The menu item that was selected or moved to
 * @param selected true if the item was selected, false if it was moved to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void mainMenuCb(const char* label, bool selected, uint32_t settingVal)
{
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
        else if (label == colorchordMode.modeName)
        {
            switchToSwadgeMode(&colorchordMode);
        }
        else if (label == shooterMode.modeName)
        {
            switchToSwadgeMode(&shooterMode);
        }
        else if (label == rayMode.modeName)
        {
            switchToSwadgeMode(&rayMode);
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
            bzrPlayBgm(&mainMenu->jingle);
        }
        else if (sfxVolSettingLabel == label)
        {
            setSfxVolumeSetting(settingVal);
            bzrPlaySfx(&mainMenu->jingle);
        }
        else if (micSettingLabel == label)
        {
            setMicGainSetting(settingVal);
        }
        else if (screenSaverSettingLabel == label)
        {
            setScreensaverTimeSetting(settingVal);
        }
    }
}
