//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"

#include "mainMenu.h"

#include "accelTest.h"
#include "breakout.h"
#include "colorchord.h"
#include "dance.h"
#include "demoMode.h"
#include "factoryTest.h"
#include "gamepad.h"
#include "jukebox.h"
#include "lumberjack.h"
#include "mode_paint.h"
#include "mode_platformer.h"
#include "mode_ray.h"
#include "modeTimer.h"
#include "paint_share.h"
#include "pushy.h"
#include "slideWhistle.h"
#include "flight.h"
#include "touchTest.h"
#include "tunernome.h"
#include "mode_credits.h"
#include "mode_pinball.h"
#include "ringsAndGems.h"

#include "settingsManager.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;
    menu_t* secretsMenu;
    menuLogbookRenderer_t* renderer;
    font_t logbook;
    song_t jingle;
    song_t fanfare;
    int32_t lastBgmVol;
    int32_t lastSfxVol;
    int32_t cheatCodeIdx;
    bool debugMode;
    bool fanfarePlaying;
    bool resetConfirmShown;
    int32_t autoLightDanceTimer;
} mainMenu_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void mainMenuEnterMode(void);
static void mainMenuExitMode(void);
static void mainMenuMainLoop(int64_t elapsedUs);
static void mainMenuCb(const char* label, bool selected, uint32_t settingVal);
void addSecretsMenu(void);

//==============================================================================
// Variables
//==============================================================================

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
const char mainMenuName[]                       = "Main Menu 1.1";
static const char mainMenuShowSecretsMenuName[] = "ShowOnMenu: ";
static const char factoryResetName[]            = "Factory Reset";
static const char confirmResetName[]            = "! Confirm Reset !";

swadgeMode_t mainMenuMode = {
    .modeName                 = mainMenuName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = true,
    .overrideSelectBtn        = true,
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

static const int16_t cheatCode[] = {
    PB_UP, PB_UP, PB_DOWN, PB_DOWN, PB_LEFT, PB_RIGHT, PB_LEFT, PB_RIGHT, PB_B, PB_A,
};

static const int32_t showSecretsMenuSettingValues[] = {
    SHOW_SECRETS,
    HIDE_SECRETS,
};

static const char* const showSecretsMenuSettingOptions[] = {
    "Show",
    "Hide",
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
    loadSong("item.sng", &mainMenu->fanfare, false);

    // Allocate the menu
    mainMenu->menu = initMenu(mainMenuName, mainMenuCb);

    // Add single items
    mainMenu->menu = startSubMenu(mainMenu->menu, "Games");
    addSingleItemToMenu(mainMenu->menu, ragMode.modeName);
    addSingleItemToMenu(mainMenu->menu, pinballMode.modeName);
    addSingleItemToMenu(mainMenu->menu, rayMode.modeName);
    addSingleItemToMenu(mainMenu->menu, breakoutMode.modeName);
    addSingleItemToMenu(mainMenu->menu, lumberjackMode.modeName);
    addSingleItemToMenu(mainMenu->menu, pushyMode.modeName);
    addSingleItemToMenu(mainMenu->menu, flightMode.modeName);
    addSingleItemToMenu(mainMenu->menu, modePlatformer.modeName);
    mainMenu->menu = endSubMenu(mainMenu->menu);

    mainMenu->menu = startSubMenu(mainMenu->menu, "Music");
    addSingleItemToMenu(mainMenu->menu, colorchordMode.modeName);
    addSingleItemToMenu(mainMenu->menu, jukeboxMode.modeName);
    addSingleItemToMenu(mainMenu->menu, slideWhistleMode.modeName);
    addSingleItemToMenu(mainMenu->menu, tunernomeMode.modeName);
    mainMenu->menu = endSubMenu(mainMenu->menu);

    mainMenu->menu = startSubMenu(mainMenu->menu, "Utilities");
    addSingleItemToMenu(mainMenu->menu, danceMode.modeName);
    addSingleItemToMenu(mainMenu->menu, modePaint.modeName);
    addSingleItemToMenu(mainMenu->menu, gamepadMode.modeName);
    addSingleItemToMenu(mainMenu->menu, timerMode.modeName);
    mainMenu->menu = endSubMenu(mainMenu->menu);

    addSingleItemToMenu(mainMenu->menu, modeCredits.modeName);

    // Start a submenu for settings
    mainMenu->menu = startSubMenu(mainMenu->menu, settingsLabel);
    // Get the bounds and current settings to build this menu
    addSettingsItemToMenu(mainMenu->menu, tftSettingLabel, getTftBrightnessSettingBounds(), getTftBrightnessSetting());
    addSettingsItemToMenu(mainMenu->menu, ledSettingLabel, getLedBrightnessSettingBounds(), getLedBrightnessSetting());
    addSettingsItemToMenu(mainMenu->menu, bgmVolSettingLabel, getBgmVolumeSettingBounds(), getBgmVolumeSetting());
    addSettingsItemToMenu(mainMenu->menu, sfxVolSettingLabel, getSfxVolumeSettingBounds(), getSfxVolumeSetting());
    addSettingsItemToMenu(mainMenu->menu, micSettingLabel, getMicGainSettingBounds(), getMicGainSetting());

    // These are just used for playing the sound only when the setting changes
    mainMenu->lastBgmVol = getBgmVolumeSetting();
    mainMenu->lastSfxVol = getSfxVolumeSetting();

    addSettingsOptionsItemToMenu(mainMenu->menu, screenSaverSettingsLabel, screenSaverSettingsOptions,
                                 screenSaverSettingsValues, ARRAY_SIZE(screenSaverSettingsValues),
                                 getScreensaverTimeSettingBounds(), getScreensaverTimeSetting());
    // End the submenu for settings
    mainMenu->menu = endSubMenu(mainMenu->menu);

    if (getShowSecretsMenuSetting() == SHOW_SECRETS)
    {
        addSecretsMenu();
    }

    // Show the battery on the main menu
    setShowBattery(mainMenu->menu, true);

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
    freeSong(&mainMenu->fanfare);

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
    // Increment this timer
    mainMenu->autoLightDanceTimer += elapsedUs;
    // If 10s have elapsed with no user input
    if (getScreensaverTimeSetting() != 0 && mainMenu->autoLightDanceTimer >= (getScreensaverTimeSetting() * 1000000))
    {
        // Switch to the LED dance mode
        switchToSwadgeMode(&danceMode);
        return;
    }

    // Pass all button events to the menu
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Any button event resets this timer
        mainMenu->autoLightDanceTimer = 0;

        if ((!mainMenu->debugMode) && (evt.down))
        {
            if (evt.button == cheatCode[mainMenu->cheatCodeIdx])
            {
                mainMenu->cheatCodeIdx++;

                if (mainMenu->cheatCodeIdx >= ARRAY_SIZE(cheatCode))
                {
                    mainMenu->cheatCodeIdx = 0;
                    soundPlayBgm(&mainMenu->fanfare, BZR_STEREO);
                    mainMenu->fanfarePlaying = true;

                    // Return to the top level menu
                    while (mainMenu->menu->parentMenu)
                    {
                        mainMenu->menu = mainMenu->menu->parentMenu;
                    }

                    addSecretsMenu();

                    return;
                }

                // Do not forward the A or START in the cheat code to the rest of the mode
                if (evt.button == PB_A || evt.button == PB_B)
                {
                    return;
                }
            }
            else
            {
                mainMenu->cheatCodeIdx = 0;
            }
        }
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

    // Stop the buzzer when changing volume, not for fanfare
    if (false == mainMenu->fanfarePlaying)
    {
        soundStop(true);
    }

    if (selected)
    {
        // These items enter other modes, so they must be selected
        if (label == accelTestMode.modeName)
        {
            switchToSwadgeMode(&accelTestMode);
        }
        else if (label == breakoutMode.modeName)
        {
            switchToSwadgeMode(&breakoutMode);
        }
        else if (label == colorchordMode.modeName)
        {
            switchToSwadgeMode(&colorchordMode);
        }
        else if (label == danceMode.modeName)
        {
            switchToSwadgeMode(&danceMode);
        }
        else if (label == demoMode.modeName)
        {
            switchToSwadgeMode(&demoMode);
        }
        else if (label == factoryTestMode.modeName)
        {
            switchToSwadgeMode(&factoryTestMode);
        }
        else if (label == factoryResetName)
        {
            if (!mainMenu->resetConfirmShown)
            {
                mainMenu->resetConfirmShown = true;
                removeSingleItemFromMenu(mainMenu->menu, mnuBackStr);
                addSingleItemToMenu(mainMenu->secretsMenu, confirmResetName);
                addSingleItemToMenu(mainMenu->menu, mnuBackStr);
            }
        }
        else if (label == confirmResetName)
        {
            if (eraseNvs())
            {
                switchToSwadgeMode(&factoryTestMode);
            }
            else
            {
                switchToSwadgeMode(&mainMenuMode);
            }
        }
        else if (label == gamepadMode.modeName)
        {
            switchToSwadgeMode(&gamepadMode);
        }
        else if (label == jukeboxMode.modeName)
        {
            switchToSwadgeMode(&jukeboxMode);
        }
        else if (label == lumberjackMode.modeName)
        {
            switchToSwadgeMode(&lumberjackMode);
        }
        else if (label == modePaint.modeName)
        {
            switchToSwadgeMode(&modePaint);
        }
        else if (label == timerMode.modeName)
        {
            switchToSwadgeMode(&timerMode);
        }
        else if (label == modePlatformer.modeName)
        {
            switchToSwadgeMode(&modePlatformer);
        }
        else if (label == pushyMode.modeName)
        {
            switchToSwadgeMode(&pushyMode);
        }
        else if (label == rayMode.modeName)
        {
            switchToSwadgeMode(&rayMode);
        }
        else if (label == pinballMode.modeName)
        {
            switchToSwadgeMode(&pinballMode);
        }
        else if (label == flightMode.modeName)
        {
            switchToSwadgeMode(&flightMode);
        }
        else if (label == slideWhistleMode.modeName)
        {
            switchToSwadgeMode(&slideWhistleMode);
        }
        else if (label == touchTestMode.modeName)
        {
            switchToSwadgeMode(&touchTestMode);
        }
        else if (label == tunernomeMode.modeName)
        {
            switchToSwadgeMode(&tunernomeMode);
        }
        else if (label == modeCredits.modeName)
        {
            switchToSwadgeMode(&modeCredits);
        }
        else if (label == ragMode.modeName)
        {
            switchToSwadgeMode(&ragMode);
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
            if (settingVal != mainMenu->lastBgmVol)
            {
                mainMenu->lastBgmVol = settingVal;
                setBgmVolumeSetting(settingVal);
                soundPlayBgm(&mainMenu->jingle, BZR_STEREO);
                mainMenu->fanfarePlaying = false;
            }
        }
        else if (sfxVolSettingLabel == label)
        {
            if (settingVal != mainMenu->lastSfxVol)
            {
                mainMenu->lastSfxVol = settingVal;
                setSfxVolumeSetting(settingVal);
                soundPlaySfx(&mainMenu->jingle, BZR_STEREO);
                mainMenu->fanfarePlaying = false;
            }
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

void addSecretsMenu(void)
{
    mainMenu->debugMode = true;

    // Add the secrets menu
    mainMenu->menu        = startSubMenu(mainMenu->menu, "Secrets");
    mainMenu->secretsMenu = mainMenu->menu;
    addSingleItemToMenu(mainMenu->menu, "Git Hash: " GIT_SHA1);
    addSettingsOptionsItemToMenu(mainMenu->menu, mainMenuShowSecretsMenuName, showSecretsMenuSettingOptions,
                                 showSecretsMenuSettingValues, ARRAY_SIZE(showSecretsMenuSettingOptions),
                                 getShowSecretsMenuSettingBounds(), getShowSecretsMenuSetting());
    addSingleItemToMenu(mainMenu->menu, demoMode.modeName);
    addSingleItemToMenu(mainMenu->menu, accelTestMode.modeName);
    addSingleItemToMenu(mainMenu->menu, touchTestMode.modeName);
    addSingleItemToMenu(mainMenu->menu, factoryTestMode.modeName);
    addSingleItemToMenu(mainMenu->menu, factoryResetName);
    mainMenu->menu = endSubMenu(mainMenu->menu);
}
