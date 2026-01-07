//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"

#include "mainMenu.h"

#include "modeIncludeList.h"

#include "settingsManager.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;
    menuMegaRenderer_t* renderer;
    midiFile_t fanfare;
#if defined(SW_VOL_CONTROL)
    midiFile_t jingle;
    int32_t lastBgmVol;
    int32_t lastSfxVol;
#endif
    int32_t cheatCodeIdx;
    bool debugMode;
#if defined(SW_VOL_CONTROL)
    bool fanfarePlaying;
#endif
    int32_t autoLightDanceTimer;

    bool modeEnterTrophyShowing;
    const swadgeMode_t* pendingMode;
} mainMenu_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void mainMenuEnterMode(void);
static void mainMenuExitMode(void);
static void mainMenuMainLoop(int64_t elapsedUs);
static bool mainMenuCb(const char* label, bool selected, uint32_t settingVal);
void addSecretsMenu(void);
#if defined(CONFIG_FACTORY_TEST_NORMAL)
static void fanfareFinishedCb(void);
#endif
static bool _winTrophy(swadgeMode_t* sm);

//==============================================================================
// Variables
//==============================================================================

const trophyData_t mainMenuTrophies[] = {
    {
        .title       = "Wait, this is a thing?!",
        .description = "Unlocked the secret menu",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
        .hidden      = true,
        .identifier  = NULL,
    },
    {
        .title       = "LEMONS!",
        .description = "Played Mega Pulse EX for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &modePlatformer,
    },
    {
        .title       = "Day -1",
        .description = "Played Cosplay Crunch for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &cosCrunchMode,
    },
    {
        .title       = "Get it!",
        .description = "Played Swadge It! for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &swadgeItMode,
    },
    {
        .title       = "Swordfish isn't always the password",
        .description = "Played Swadgedoku for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &swadgedokuMode,
    },
    {
        .title       = "PulseMan and the King... Man",
        .description = "Played Alpha Pulse for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &danceNetworkMode,
    },
    {
        .title       = "Runnin' and runnin'",
        .description = "Played Robo Runner for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &roboRunnerMode,
    },
    {
        .title       = "It's pronounced 'Pie-Cross'",
        .description = "Played Picross for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &modePicross,
    },
    {
        .title       = "What's our vector, Victor?",
        .description = "Played Vector Tanks for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &artilleryMode,
    },
    {
        .title       = "Daft Punk would be proud",
        .description = "Opened Colorchord for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &colorchordMode,
    },
    {
        .title       = "A jukebox hero",
        .description = "Opened Jukebox for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &jukeboxMode,
    },
    {
        .title       = "mm2wood.mid",
        .description = "Opened the Sequencer for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &sequencerMode,
    },
    {
        .title       = "Who needs a tuning fork?",
        .description = "Opened Tunernome for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &tunernomeMode,
    },
    // {
    //     .title       = "The smallest player",
    //     .description = "Opened MIDI Player for the first time",
    //     .image       = NO_IMAGE_SET,
    //     .type        = TROPHY_TYPE_TRIGGER,
    //     .difficulty  = TROPHY_DIFF_EASY,
    //     .maxVal      = 1,
    //     .identifier  = &synthMode,
    // },
    {
        .title       = "The song of my people",
        .description = "Opened Swadgetamatone for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &swadgetamatoneMode,
    },
    {
        .title       = "Blinded by the lights",
        .description = "Opened Light dances on purpose for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &danceMode,
    },
    {
        .title       = "I still like physical dice",
        .description = "Opened Dice Roller for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &modeDiceRoller,
    },
    {
        .title       = "It can't hit the corner",
        .description = "Opened Bouncy items for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &bouncyMode,
    },
    {
        .title       = "...Was once not enough?",
        .description = "Opened the intro again for some reason",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &introMode,
    },
    {
        .title       = "Switch Amateur Controller",
        .description = "Opened Gamepad for the first time",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .identifier  = &gamepadMode,
    },
    {
        .title       = "MANHUNT!",
        .description = "Opened Mascot Madness for the first time.",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
        .identifier  = &findingFacesMode,
    },
};

const trophySettings_t menuTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = mainMenuName,
};

const trophyDataList_t menuTrophyData = {
    .settings = &menuTrophySettings,
    .list     = mainMenuTrophies,
    .length   = ARRAY_SIZE(mainMenuTrophies),
};

// It's good practice to declare immutable strings as const so they get placed in ROM, not RAM
const char mainMenuName[] = "Main Menu";
const char mainMenuTitle[]
#if defined(CONFIG_HARDWARE_SWADGEBOY)
    = "Swadgeboy 3.1";
#else
    = "Swadge 3.1";
#endif
static const char mainMenuShowSecretsMenuName[] = "Secrets In Menu: ";
static const char factoryResetName[]            = "Factory Reset";
static const char confirmResetName[]            = "! Confirm Reset !";

swadgeMode_t mainMenuMode = {
    .modeName                 = mainMenuName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = true,
    .fnEnterMode              = mainMenuEnterMode,
    .fnExitMode               = mainMenuExitMode,
    .fnMainLoop               = mainMenuMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .trophyData               = &menuTrophyData,
};

mainMenu_t* mainMenu;

static const char tftSettingLabel[] = "TFT";
static const char ledSettingLabel[] = "LED";
#if defined(SW_VOL_CONTROL)
static const char bgmVolSettingLabel[] = "BGM";
static const char sfxVolSettingLabel[] = "SFX";
#endif
const char micSettingLabel[]                 = "MIC";
static const char screenSaverSettingsLabel[] = "Screensaver: ";

#if defined(CONFIG_FACTORY_TEST_NORMAL)

static const char settingsLabel[] = "Settings";

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

#endif

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
    // Turn off DAC, for now...
    setDacShutdown(true);

    // Allocate memory for the mode
    mainMenu = heap_caps_calloc(1, sizeof(mainMenu_t), MALLOC_CAP_8BIT);

    // Load a song for when the volume changes
#if defined(SW_VOL_CONTROL)
    loadMidiFile(JINGLE_MID, &mainMenu->jingle, false);
#endif
    loadMidiFile(SECRET_MID, &mainMenu->fanfare, true);
    initGlobalMidiPlayer();
    midiGmOn(globalMidiPlayerGet(MIDI_BGM));

    // Allocate the menu
    mainMenu->menu = initMenu(mainMenuTitle, mainMenuCb);

#if defined(CONFIG_FACTORY_TEST_NORMAL)
    // Initialize all the modes in modeList
    modeListSetMenu(mainMenu->menu);

    // Start a submenu for settings
    mainMenu->menu = startSubMenu(mainMenu->menu, settingsLabel);
    // Get the bounds and current settings to build this menu
    addSettingsItemToMenu(mainMenu->menu, tftSettingLabel, getTftBrightnessSettingBounds(), getTftBrightnessSetting());
    addSettingsItemToMenu(mainMenu->menu, ledSettingLabel, getLedBrightnessSettingBounds(), getLedBrightnessSetting());
    #if defined(SW_VOL_CONTROL)
    addSettingsItemToMenu(mainMenu->menu, bgmVolSettingLabel, getBgmVolumeSettingBounds(), getBgmVolumeSetting());
    addSettingsItemToMenu(mainMenu->menu, sfxVolSettingLabel, getSfxVolumeSettingBounds(), getSfxVolumeSetting());
    #endif
    addSettingsItemToMenu(mainMenu->menu, micSettingLabel, getMicGainSettingBounds(), getMicGainSetting());

    #if defined(SW_VOL_CONTROL)
    // These are just used for playing the sound only when the setting changes
    mainMenu->lastBgmVol = getBgmVolumeSetting();
    mainMenu->lastSfxVol = getSfxVolumeSetting();
    #endif

    addSettingsOptionsItemToMenu(mainMenu->menu, screenSaverSettingsLabel, screenSaverSettingsOptions,
                                 screenSaverSettingsValues, ARRAY_SIZE(screenSaverSettingsValues),
                                 getScreensaverTimeSettingBounds(), getScreensaverTimeSetting());
    // End the submenu for settings
    mainMenu->menu = endSubMenu(mainMenu->menu);

    if (getShowSecretsMenuSetting() == SHOW_SECRETS)
    {
        addSecretsMenu();
    }

#endif

    // Show the battery on the main menu
    setShowBattery(mainMenu->menu, true);

    // Initialize menu renderer
    mainMenu->renderer = initMenuMegaRenderer(NULL, NULL, NULL);
}

/**
 * @brief Deinitialize the main menu mode
 */
static void mainMenuExitMode(void)
{
    // Deinit menu
    deinitMenu(mainMenu->menu);

    // Deinit renderer
    deinitMenuMegaRenderer(mainMenu->renderer);

    // Free the song
#if defined(SW_VOL_CONTROL)
    unloadMidiFile(&mainMenu->jingle);
#endif
    unloadMidiFile(&mainMenu->fanfare);

    // Free mode memory
    heap_caps_free(mainMenu);
}

/**
 * @brief Main loop for the main menu. Simply handles menu input and draws the menu
 *
 * @param elapsedUs unused
 */
static void mainMenuMainLoop(int64_t elapsedUs)
{
#if defined(CONFIG_FACTORY_TEST_NORMAL)
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
                    setDacShutdown(false);
                    globalMidiPlayerPlaySongCb(&mainMenu->fanfare, MIDI_BGM, fanfareFinishedCb);
    #if defined(SW_VOL_CONTROL)
                    mainMenu->fanfarePlaying = true;
    #endif
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

        // Only accept button input if a trophy isn't showing
        if (false == mainMenu->modeEnterTrophyShowing)
        {
            mainMenu->menu = menuButton(mainMenu->menu, evt);
        }
    }

#endif

    // Draw the menu
    drawMenuMega(mainMenu->menu, mainMenu->renderer, elapsedUs);

#if defined(CONFIG_FACTORY_TEST_NORMAL)

    // If a trophy was showing, but the animation is done
    if (mainMenu->modeEnterTrophyShowing && !isTrophyDrawing())
    {
        // Finally switch to the pending mode
        switchToSwadgeMode(mainMenu->pendingMode);
    }

#elif defined(CONFIG_FACTORY_TEST_WARNING)

    font_t* font         = mainMenu->renderer->menuFont;
    const char warning[] = "Take me to VR Zone to get flashed please!";
    #define INNER_MARGIN 16
    int16_t xOff         = 20 + INNER_MARGIN + 1;
    int16_t yOff         = 54 + INNER_MARGIN + 32 + 1;
    drawTextWordWrapCentered(font, c000, warning, &xOff, &yOff, //
                             TFT_WIDTH - 24 - INNER_MARGIN + 1, //
                             TFT_HEIGHT - 23 - INNER_MARGIN + 1);

    xOff = 20 + INNER_MARGIN;
    yOff = 54 + INNER_MARGIN + 32;
    drawTextWordWrapCentered(font, c555, warning, &xOff, &yOff, //
                             TFT_WIDTH - 24 - INNER_MARGIN,     //
                             TFT_HEIGHT - 23 - INNER_MARGIN);

#endif
}

#if defined(CONFIG_FACTORY_TEST_NORMAL)
/**
 * @brief Callback after the fanfare is done playing to disable the DAC again
 */
static void fanfareFinishedCb(void)
{
    setDacShutdown(true);
}
#endif

/**
 * @brief Callback for when menu items are selected
 *
 * @param label The menu item that was selected or moved to
 * @param selected true if the item was selected, false if it was moved to
 * @param settingVal The value of the setting, if the menu item is a settings item
 * @return true to go up a menu level, false to remain here
 */
static bool mainMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    // Stop the buzzer first no matter what, so that it turns off
    // if we scroll away from the BGM or SFX settings.

#if defined(SW_VOL_CONTROL)
    // Stop the buzzer when changing volume, not for fanfare
    if (false == mainMenu->fanfarePlaying)
    {
        soundStop(true);
    }
#endif

    if (selected)
    {
        // These items enter other modes, so they must be selected
        for (int i = 0; i < modeListGetCount(); i++)
        {
            swadgeMode_t* current = allSwadgeModes[i];
            if (label == current->modeName)
            {
                // If entering this mode won a trophy
                if (_winTrophy(current))
                {
                    // Wait for the trophy to be shown before switching modes
                    mainMenu->modeEnterTrophyShowing = true;
                    mainMenu->pendingMode            = current;
                }
                else
                {
                    // Otherwise immediately enter the mode
                    switchToSwadgeMode(current);
                }
            }
        }
        if (label == tCaseMode.modeName)
        {
            switchToSwadgeMode(&tCaseMode);
        }
        if (label == confirmResetName)
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
#if defined(SW_VOL_CONTROL)
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
#endif
        else if (micSettingLabel == label)
        {
            setMicGainSetting(settingVal);
        }
        else if (screenSaverSettingsLabel == label)
        {
            setScreensaverTimeSetting(settingVal);
        }
        else if (mainMenuShowSecretsMenuName == label)
        {
            setShowSecretsMenuSetting(settingVal);
        }
    }
    return false;
}

void addSecretsMenu(void)
{
    mainMenu->debugMode = true;

    // Return to the root
    while (mainMenu->menu->parentMenu)
    {
        mainMenu->menu = mainMenu->menu->parentMenu;
    }

    // Add the secrets menu
    mainMenu->menu = startSubMenu(mainMenu->menu, "Secrets");

    addSingleItemToMenu(mainMenu->menu, "Git Hash: " GIT_SHA1);
    addSettingsOptionsItemToMenu(mainMenu->menu, mainMenuShowSecretsMenuName, showSecretsMenuSettingOptions,
                                 showSecretsMenuSettingValues, ARRAY_SIZE(showSecretsMenuSettingOptions),
                                 getShowSecretsMenuSettingBounds(), getShowSecretsMenuSetting());

    modeListAddSecretMenuModes(mainMenu->menu);

    mainMenu->menu = startSubMenu(mainMenu->menu, factoryResetName);
    addSingleItemToMenu(mainMenu->menu, confirmResetName);
    // Back is automatically added
    mainMenu->menu = endSubMenu(mainMenu->menu);

    // End the secrets menu
    mainMenu->menu = endSubMenu(mainMenu->menu);

    // Get a trophy
    _winTrophy(NULL);
}

static bool _winTrophy(swadgeMode_t* sm)
{
    for (int32_t idx = 0; idx < ARRAY_SIZE(mainMenuTrophies); idx++)
    {
        if (mainMenuTrophies[idx].identifier == sm)
        {
            return trophyUpdate(&mainMenuTrophies[idx], 1, true);
        }
    }
    return false;
}
