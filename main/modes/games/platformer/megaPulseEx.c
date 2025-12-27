/**
 * @file megaPulseEx.c
 * @author J.Vega (JVeg199X)
 * @brief Mega Pulse EX (JPV-MGSW-004-MG)
 * @date 2025-04-27
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "megaPulseEx.h"
#include "esp_random.h"
#include <esp_heap_caps.h>

#include "mega_pulse_ex_typedef.h"
#include "mgWsgManager.h"
#include "mgTilemap.h"
#include "mgGameData.h"
#include "mgEntityManager.h"
#include "mgLeveldef.h"
#include "mgCutscenes.h"

#include "hdw-led.h"
#include "palette.h"
#include "hdw-nvs.h"
#include "mgSoundManager.h"
#include <inttypes.h>
#include "mainMenu.h"
#include "fill.h"
#include "cutscene.h"
#include "highScores.h"

//==============================================================================
// Constants
//==============================================================================
#define BIG_SCORE    4000000UL
#define BIGGER_SCORE 10000000UL
#define FAST_TIME    900 // 15 minutes

const char platformerName[] = "Mega Pulse EX";

static const paletteColor_t highScoreNewEntryColors[4] = {c050, c055, c005, c055};

static const paletteColor_t redColors[4]    = {c501, c540, c550, c540};
static const paletteColor_t yellowColors[4] = {c550, c331, c550, c555};
static const paletteColor_t greenColors[4]  = {c555, c051, c030, c051};
static const paletteColor_t cyanColors[4]   = {c055, c455, c055, c033};
static const paletteColor_t purpleColors[4] = {c213, c535, c555, c535};
static const paletteColor_t rgbColors[4]    = {c500, c050, c005, c050};

static const int16_t cheatCode[11]
    = {PB_UP, PB_UP, PB_DOWN, PB_DOWN, PB_LEFT, PB_RIGHT, PB_LEFT, PB_RIGHT, PB_B, PB_A, PB_START};

//==============================================================================
// Functions Prototypes
//==============================================================================

void platformerEnterMode(void);
void platformerExitMode(void);
void platformerMainLoop(int64_t elapsedUs);

//==============================================================================
// Structs
//==============================================================================

typedef void (*gameUpdateFuncton_t)(platformer_t* self);
struct platformer_t
{
    font_t font;
    mgWsgManager_t wsgManager;

    mgTilemap_t tilemap;
    mgEntityManager_t entityManager;
    mgGameData_t gameData;

    mgSoundManager_t soundManager;

    uint8_t menuState;
    uint8_t menuSelection;
    uint8_t cheatCodeIdx;

    int16_t btnState;
    int16_t prevBtnState;

    int32_t frameTimer;

    //platformerHighScores_t highScores;
    highScores_t highScores;
    swadgesona_t sonas[NUM_PLATFORMER_HIGH_SCORES];

    platformerUnlockables_t unlockables;
    bool easterEgg;

    gameUpdateFuncton_t update;

    menuMegaRenderer_t* menuRenderer;
    menu_t* menu;
};

//==============================================================================
// Function Prototypes
//==============================================================================
void drawPlatformerHud(font_t* font, mgGameData_t* gameData);
void drawPlatformerTitleScreen(font_t* font, mgGameData_t* gameData);
void changeStateReadyScreen(platformer_t* self);
void updateCutsceneScreen(platformer_t* self);
void drawCutsceneScreen(platformer_t* self);
void updateReadyScreen(platformer_t* self);
void drawReadyScreen(font_t* font, mgGameData_t* gameData);
void changeStateGame(platformer_t* self);
void detectGameStateChange(platformer_t* self);
void detectBgmChange(platformer_t* self);
void changeStateDead(platformer_t* self);
void updateDead(platformer_t* self);
void changeStateGameOver(platformer_t* self);
void updateGameOver(platformer_t* self);
void drawGameOver(font_t* font, mgGameData_t* gameData);
void changeStateTitleScreen(platformer_t* self);
void changeStateLevelClear(platformer_t* self);
void updateLevelClear(platformer_t* self);
void drawLevelClear(font_t* font, mgGameData_t* gameData);
void changeStateGameClear(platformer_t* self);
void changeStateCutscene(platformer_t* self);
void updateGameClear(platformer_t* self);
void drawGameClear(font_t* font, mgGameData_t* gameData);
void initializePlatformerHighScores(platformer_t* self);
void loadPlatformerHighScores(platformer_t* self);
void savePlatformerHighScores(platformer_t* self);
void initializePlatformerUnlockables(platformer_t* self);
void loadPlatformerUnlockables(platformer_t* self);
void savePlatformerUnlockables(platformer_t* self);
void drawPlatformerHighScores(font_t* font, highScores_t* highScores, swadgesona_t* sonas, mgGameData_t* gameData);
uint8_t getHighScoreRank(platformerHighScores_t* highScores, uint32_t newScore);
void insertScoreIntoHighScores(platformerHighScores_t* highScores, uint32_t newScore, char newInitials[], uint8_t rank);
void changeStateNameEntry(platformer_t* self);
void updateNameEntry(platformer_t* self);
void drawNameEntry(font_t* font, mgGameData_t* gameData, uint8_t currentInitial);
void changeStateShowHighScores(platformer_t* self);
void updateShowHighScores(platformer_t* self);
void drawShowHighScores(font_t* font, uint8_t menuState);
void changeStatePause(platformer_t* self);
void updatePause(platformer_t* self);
void drawPause(font_t* font);
uint16_t getLevelIndex(uint8_t world, uint8_t level);
void changeStateMainMenu(platformer_t* self);
void mgBuildMainMenu(platformer_t* self);
void mgUpdateMainMenu(platformer_t* self);
static void mg_backgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
void changeStateLevelSelect(platformer_t* self);
void updateLevelSelect(platformer_t* self);
void drawLevelSelect(platformer_t* self);
static void megaPulseAddToSwadgePassPacket(swadgePassPacket_t* packet);
static int32_t megaPulseGetSwadgePassHighScore(const swadgePassPacket_t* packet);
static void megaPulseSetSwadgePassHighScore(swadgePassPacket_t* packet, int32_t highScore);

//==============================================================================
// Variables
//==============================================================================

// Trophy definitions for platformer mode
const trophyData_t platformerTrophies[] = {
    {
        .title       = "Cured Kinetic Donut",
        .description = "Favorite genre: Funk",
        .image       = TROPHY_KINETIC_DONUT_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Cured Grind Pangolin",
        .description = "Favorite genre: Ska",
        .image       = TROPHY_GRIND_PANGOLIN_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Cured Sever Yataga",
        .description = "Favorite genre: EDM",
        .image       = TROPHY_SEVER_YATAGA_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Rescued Ember Demon",
        .description = "But did Trash Man survive?",
        .image       = TROPHY_EMBER_DEMON_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Cured Smash Gorilla",
        .description = "Favorite genre: Salsa",
        .image       = TROPHY_SMASH_GORILLA_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Cured Deadeye Chirpzi",
        .description = "Favorite genre: Metal",
        .image       = TROPHY_DEADEY_CHIRPZI_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Cured Drain Bat",
        .description = "Favorite genre: Classical",
        .image       = TROPHY_DRAIN_BAT_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Cured Flare Gryffyn",
        .description = "Favorite genre: Classic Rock",
        .image       = TROPHY_FLARE_GRYFFYN_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Cured Bigma", // this has been moved to clearing the boss rush
        .description = "Favorite genre: Corruption?",
        .image       = TROPHY_BIGMA_WSG, // need 36 x 36 boss images later
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "Defeated Hank Waddle",
        .description = "Favorite genre: Silence",
        .image       = TROPHY_HANK_WADDLE_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
    },
    {
        .title       = "Cured Trash Man",
        .description = "Favorite genre: JAZZ",
        .image       = TROPHY_TRASH_MAN_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
    },
    {
        .title       = "Mega Player",
        .description = "Scored 4 million!",
        .image       = SILVER_TROPHY_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
    },
    {
        .title       = "Mega Player EX",
        .description = "Scored 10 million!",
        .image       = GOLD_TROPHY_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 1,
    },
    {
        .title       = "1 Credit Cleared!",
        .description = "Clear in one session without Game Over!",
        .image       = SILVER_TROPHY_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
    },
    {
        .title       = "Not just fast, magFAST!",
        .description = "1CC'd in 15 gameplay minutes!",
        .image       = GOLD_TROPHY_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 1,
    }
};

// Individual mode settings
const trophySettings_t platformerTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 11,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = "Alpha Pulse",
};

// This is passed to the swadgeMode_t
const trophyDataList_t platformerTrophyData = {
    .settings = &platformerTrophySettings,
    .list     = platformerTrophies,
    .length   = ARRAY_SIZE(platformerTrophies),
};

platformer_t* platformer = NULL;

swadgeMode_t modePlatformer = {
    .modeName                 = platformerName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = platformerEnterMode,
    .fnExitMode               = platformerExitMode,
    .fnMainLoop               = platformerMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = mg_backgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAddToSwadgePassPacket  = megaPulseAddToSwadgePassPacket,
    .trophyData               = &platformerTrophyData, // This line activates the trophy for this mode
};

// #define NUM_LEVELS 16

led_t platLeds[CONFIG_NUM_LEDS];

static const char str_get_ready[]    = "LET'S GOOOOOO!";
static const char str_get_ready_2[]  = "WE'RE SO BACK!";
static const char str_time_up[]      = "-Time Up!-";
static const char str_game_over[]    = "IT'S SO OVER...";
static const char str_well_done[]    = "STAGE CLEAR!";
static const char str_congrats[]     = "Congratulations!";
static const char str_initials[]     = "Enter your initials!";
static const char str_hbd[]          = "Happy Birthday, Evelyn!";
static const char str_registrated[]  = "Your name registrated.";
static const char str_do_your_best[] = "MEGA PLAYERS";
static const char str_pause[]        = "-Pause-";

static const char KEY_SCORES[]  = "mg_scores_swps";
static const char KEY_UNLOCKS[] = "mg_unlocks";

static const char mgMenuNewGame[]            = "New Game";
static const char mgMenuPlaceholder[]        = "-------------";
static const char mgMenuContinue[]           = "Continue";
static const char mgMenuHighScores[]         = "High Scores";
static const char mgMenuResetScores[]        = "Reset Scores";
static const char mgMenuResetProgress[]      = "Reset Progress";
static const char mgMenuExit[]               = "Exit";
static const char mgMenuSaveAndExit[]        = "Save & Exit";
static const char mgMenuStartOver[]          = "Start Over";
static const char mgMenuOptions[]            = "Options";
static const char mgMenuConfirm[]            = "Confirm";
static const char mgMenuPlayCustomLevel[]    = "Play Custom Level";
static const char mgMenuGo[]                 = "Go!!!";
static const char mgMenuSetGameState[]       = "Set Game State";
static const char mgMenuSetLevelMetadata[]   = "Set Level Metadata";
static const char mgMenuAbilityUnlockState[] = "LvlsClear";

// options menu
static const int32_t trueFalseVals[] = {
    false,
    true,
};
static const char str_cheatMode[]     = "Cheat Mode: ";
static const char str_On[]            = "On";
static const char str_Off[]           = "Off";
static const char* strs_on_off[]      = {str_Off, str_On};
static const char str_giveAbilities[] = "Click to unlock all abilities.";

static const settingParam_t mgAbilityUnlockStateSettingBounds = {.min = 0, .max = 256, .key = KEY_UNLOCKS};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
void platformerEnterMode(void)
{
    // Allocate memory for this mode
    platformer = (platformer_t*)heap_caps_calloc(1, sizeof(platformer_t), MALLOC_CAP_8BIT);
    memset(platformer, 0, sizeof(platformer_t));

    platformer->menuState     = 0;
    platformer->menuSelection = 0;
    platformer->btnState      = 0;
    platformer->prevBtnState  = 0;

    //loadPlatformerHighScores(platformer);
    platformer->highScores.highScoreCount = NUM_PLATFORMER_HIGH_SCORES;
    initHighScores(&platformer->highScores, KEY_SCORES);
    loadPlatformerUnlockables(platformer);
    // if (platformer->highScores.initials[0][0] == 'E' && platformer->highScores.initials[0][1] == 'F'
    //     && platformer->highScores.initials[0][2] == 'V')
    // {
    //     platformer->easterEgg = true;
    // }

    list_t swadgePasses = {0};
    // It's okay to get already-used passes, since the high score table only saves one per SP user.
    getSwadgePasses(&swadgePasses, &platformer, true);
    saveHighScoresFromSwadgePass(&platformer->highScores, KEY_SCORES, swadgePasses, megaPulseGetSwadgePassHighScore);
    freeSwadgePasses(&swadgePasses);

    loadFont(MEGAMAX_JONES_FONT, &platformer->font, false);
    platformer->menuRenderer = initMenuMegaRenderer(NULL, NULL, NULL);

    mg_initializeWsgManager(&(platformer->wsgManager));

    mg_initializeTileMap(&(platformer->tilemap), &(platformer->wsgManager));

    mg_loadWsgSet(&(platformer->wsgManager), leveldef[1].defaultWsgSetIndex);
    mg_loadMapFromFile(&(platformer->tilemap), leveldef[1].filename, &platformer->entityManager);

    mg_initializeSoundManager(&(platformer->soundManager));

    mg_initializeGameData(&(platformer->gameData), &(platformer->soundManager));
    mg_initializeEntityManager(&(platformer->entityManager), &(platformer->wsgManager), &(platformer->tilemap),
                               &(platformer->gameData), &(platformer->soundManager));

    platformer->tilemap.entityManager    = &(platformer->entityManager);
    platformer->tilemap.tileSpawnEnabled = true;

    if (platformer->gameData.cutscene == NULL)
    {
        platformer->gameData.cutscene = initCutscene(goToReadyScreen, CUTSCENE_NEXT_0_WSG, 2);

        // Pulse
        addCutsceneStyle(platformer->gameData.cutscene, c000, PULSE_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG, "Pulse", 5, true,
                         true, true);
        setMidiParams(platformer->gameData.cutscene, 0, 48, 0, 1000, false);
        // Sawtooth
        addCutsceneStyle(platformer->gameData.cutscene, c530, SAWTOOTH_PORTRAIT_0_WSG, TEXTBOX_SAWTOOTH_WSG, "Sawtooth",
                         4, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 1, 81, 1, 100, false);
        // Bigma
        addCutsceneStyle(platformer->gameData.cutscene, c544, BIGMA_PORTRAIT_0_WSG, TEXTBOX_BIGMA_WSG, "Bigma", 4,
                         false, true, true);
        setMidiParams(platformer->gameData.cutscene, 2, 80, -2, 2000, true);
        // TrashMan
        addCutsceneStyle(platformer->gameData.cutscene, c414, OVO_PORTRAIT_0_WSG, TEXTBOX_CORRUPTED_WSG, "Trash Man", 3,
                         false, true, true);
        setMidiParams(platformer->gameData.cutscene, 3, 36, -2, 250, false);
        /// AbilityUnlocked
        addCutsceneStyle(platformer->gameData.cutscene, c454, ABILITY_UNLOCKED_WSG, TEXTBOX_PULSE_WSG, "", 1, true,
                         true, false);
        setMidiParams(platformer->gameData.cutscene, 4, 11, 0, 250, false);
        // SystemText
        addCutsceneStyle(platformer->gameData.cutscene, c000, ABILITY_UNLOCKED_WSG, TEXTBOX_PULSE_WSG, "System Text", 1,
                         true, false, true);
        setMidiParams(platformer->gameData.cutscene, 5, 55, 0, 250, false);
        // KineticDonut
        addCutsceneStyle(platformer->gameData.cutscene, c310, KINETIC_DONUT_PORTRAIT_0_WSG, TEXTBOX_CORRUPTED_WSG,
                         "Kinetic Donut", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 6, 80, -1, 1000, false);
        // JoltLapin
        addCutsceneStyle(platformer->gameData.cutscene, c000, JOLT_LAPIN_PORTRAIT_0_WSG, TEXTBOX_CORRUPTED_WSG,
                         "Jolt Lapin", 1, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 7, 80, 1, 250, false);
        // FlareGryffyn
        addCutsceneStyle(platformer->gameData.cutscene, c310, FLARE_GRYFFYN_PORTRAIT_0_WSG, TEXTBOX_CORRUPTED_WSG,
                         "Flare Gryffyn", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 8, 24, 1, 250, false);
        // CrashTurtle
        addCutsceneStyle(platformer->gameData.cutscene, c000, CRASH_TURTLE_PORTRAIT_0_WSG, TEXTBOX_CORRUPTED_WSG,
                         "Crash Turtle", 1, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 9, 119, 1, 250, false);
        // QuestionMark
        addCutsceneStyle(platformer->gameData.cutscene, c000, ABILITY_UNLOCKED_WSG, TEXTBOX_PULSE_WSG, "??????", 1,
                         false, false, true);
        setMidiParams(platformer->gameData.cutscene, 10, 119, 1, 100, true);
        // DeadeyeChirpzi
        addCutsceneStyle(platformer->gameData.cutscene, c000, DEADEYE_CHIRPZI_PORTRAIT_0_WSG, TEXTBOX_CORRUPTED_WSG,
                         "Deadeye Chirpzi", 1, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 11, 30, 1, 80, false);
        // HankWaddle
        addCutsceneStyle(platformer->gameData.cutscene, c000, HANK_WADDLE_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG,
                         "Hank Waddle", 2, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 12, 38, 1, 250, true);
        // GrindPangolin
        addCutsceneStyle(platformer->gameData.cutscene, c310, GRIND_PANGOLIN_PORTRAIT_0_WSG, TEXTBOX_CORRUPTED_WSG,
                         "Grind Pangolin", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 13, 62, 1, 250, false);
        // DrainBat
        addCutsceneStyle(platformer->gameData.cutscene, c000, DRAIN_BAT_PORTRAIT_0_WSG, TEXTBOX_CORRUPTED_WSG,
                         "Drain Bat", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 14, 82, 2, 250, false);
        // SmashGorilla
        addCutsceneStyle(platformer->gameData.cutscene, c310, SMASH_GORILLA_PORTRAIT_0_WSG, TEXTBOX_CORRUPTED_WSG,
                         "Smash Gorilla", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 15, 29, -1, 250, false);
        // SeverYataga
        addCutsceneStyle(platformer->gameData.cutscene, c310, SEVER_YATAGA_PORTRAIT_0_WSG, TEXTBOX_CORRUPTED_WSG,
                         "Sever Yataga", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 16, 24, 1, 250, false);
        // Jasper
        addCutsceneStyle(platformer->gameData.cutscene, c000, JASPER_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG, "Jasper", 1,
                         true, true, true);
        setMidiParams(platformer->gameData.cutscene, 17, 83, 1, 250, false);
        // Ember
        addCutsceneStyle(platformer->gameData.cutscene, c000, EMBER_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG, "Ember", 1, true,
                         true, true);
        setMidiParams(platformer->gameData.cutscene, 18, 82, 1, 250, false);
        // Percy
        addCutsceneStyle(platformer->gameData.cutscene, c000, PERCY_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG, "Percy", 1, true,
                         true, true);
        setMidiParams(platformer->gameData.cutscene, 19, 38, 1, 250, false);
        // Sunny
        addCutsceneStyle(platformer->gameData.cutscene, c541, SUNNY_PORTRAIT_0_WSG, TEXTBOX_SAWTOOTH_WSG, "Sunny", 2,
                         true, true, true);
        setMidiParams(platformer->gameData.cutscene, 20, 81, 1, 100, true);
        // WarningMessage
        addCutsceneStyle(platformer->gameData.cutscene, c254, WARNING_MESSAGE_PORTRAIT_WSG, TEXTBOX_PULSE_WSG, "", 1,
                         true, true, false);
        setMidiParams(platformer->gameData.cutscene, 21, 55, 0, 100, false);
        // SawtoothPostReveal
        addCutsceneStyle(platformer->gameData.cutscene, c541, SAWTOOTH_PORTRAIT_0_WSG, TEXTBOX_SAWTOOTH_WSG, "Sawtooth",
                         4, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 22, 81, 1, 100, true);
        // BlackScreen
        addCutsceneStyle(platformer->gameData.cutscene, c555, BLACK_PORTRAIT_WSG, TEXTBOX_SAWTOOTH_WSG, "", 1, false,
                         true, false);
        setMidiParams(platformer->gameData.cutscene, 23, 11, -1, 100, false);

        /////////////////////////
        // UNCORRUPTED VERSIONS//
        /////////////////////////
        //  TrashMan
        addCutsceneStyle(platformer->gameData.cutscene, c515, OVO_PORTRAIT_0_WSG, TEXTBOX_OVO_WSG, "Trash Man", 3,
                         false, true, true);
        setMidiParams(platformer->gameData.cutscene, 24, 36, -2, 250, false);
        // KineticDonut
        addCutsceneStyle(platformer->gameData.cutscene, c310, KINETIC_DONUT_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG,
                         "Kinetic Donut", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 25, 80, -1, 1000, false);
        // JoltLapin
        addCutsceneStyle(platformer->gameData.cutscene, c000, JOLT_LAPIN_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG,
                         "Jolt Lapin", 1, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 26, 80, 1, 250, false);
        // FlareGryffyn
        addCutsceneStyle(platformer->gameData.cutscene, c310, FLARE_GRYFFYN_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG,
                         "Flare Gryffyn", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 27, 24, 1, 250, false);
        // CrashTurtle
        addCutsceneStyle(platformer->gameData.cutscene, c000, CRASH_TURTLE_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG,
                         "Crash Turtle", 1, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 28, 119, 1, 250, false);
        // DeadeyeChirpzi
        addCutsceneStyle(platformer->gameData.cutscene, c000, DEADEYE_CHIRPZI_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG, "Cho",
                         1, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 29, 17, 1, 250, false);
        // GrindPangolin
        addCutsceneStyle(platformer->gameData.cutscene, c310, GRIND_PANGOLIN_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG,
                         "Grind Pangolin", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 30, 62, 1, 250, false);
        // SmashGorilla
        addCutsceneStyle(platformer->gameData.cutscene, c310, SMASH_GORILLA_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG,
                         "Smash Gorilla", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 31, 29, -1, 250, false);
        // SeverYataga
        addCutsceneStyle(platformer->gameData.cutscene, c310, SEVER_YATAGA_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG,
                         "Sever Yataga", 1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 32, 11, 0, 100, true);
        // DrainBat
        addCutsceneStyle(platformer->gameData.cutscene, c000, DRAIN_BAT_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG, "Battrice",
                         1, false, true, true);
        setMidiParams(platformer->gameData.cutscene, 33, 82, 2, 250, false);
        // DeadeyeWithoutZip
        addCutsceneStyle(platformer->gameData.cutscene, c000, DEADEYE_CHIRPZI_WITHOUT_ZIP_PORTRAIT_0_WSG,
                         TEXTBOX_PULSE_WSG, "Cho", 1, true, true, true);
        // reduce note length, because she just lost Zip.
        setMidiParams(platformer->gameData.cutscene, 34, 17, 1, 100, false);

        //////////////////////
        // Random Schizz idk//
        //////////////////////
        // Sunny Flipped
        addCutsceneStyle(platformer->gameData.cutscene, c541, SUNNY_FLIPPED_PORTRAIT_0_WSG, TEXTBOX_SAWTOOTH_WSG,
                         "Sunny", 2, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 35, 81, 1, 100, true);
        // SawtoothRevealed Flipped
        addCutsceneStyle(platformer->gameData.cutscene, c541, SAWTOOTH_FLIPPED_PORTRAIT_0_WSG, TEXTBOX_SAWTOOTH_WSG,
                         "Sawtooth", 2, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 36, 81, 1, 100, true);
        // Sawtooth Flipped
        addCutsceneStyle(platformer->gameData.cutscene, c541, SAWTOOTH_FLIPPED_PORTRAIT_0_WSG, TEXTBOX_SAWTOOTH_WSG,
                         "Sawtooth", 2, true, true, true);
        setMidiParams(platformer->gameData.cutscene, 37, 81, 1, 100, false);
        // Hank Unrevealed
        addCutsceneStyle(platformer->gameData.cutscene, c000, ABILITY_UNLOCKED_WSG, TEXTBOX_PULSE_WSG, "??????", 1,
                         false, false, true);
        setMidiParams(platformer->gameData.cutscene, 38, 38, 1, 250, true);
        // CreditStyle
        addCutsceneStyle(platformer->gameData.cutscene, c555, CREDIT_PORTRAIT_WSG, TEXTBOX_SAWTOOTH_WSG, "", 1, false,
                         true, false);
        setMidiParams(platformer->gameData.cutscene, 39, 11, -1, 100, false);
        // Ember (no cage)
        addCutsceneStyle(platformer->gameData.cutscene, c000, EMBER_FREE_PORTRAIT_0_WSG, TEXTBOX_PULSE_WSG, "Ember", 1,
                         true, true, true);
        setMidiParams(platformer->gameData.cutscene, 40, 82, 1, 250, false);
    }

    setFrameRateUs(16666);

    changeStateMainMenu(platformer);
}

/**
 * @brief TODO
 *
 */
void platformerExitMode(void)
{
    freeFont(&platformer->font);
    mg_freeWsgManager(&(platformer->wsgManager));
    mg_freeTilemap(&(platformer->tilemap));
    mg_freeSoundManager(&(platformer->soundManager));
    mg_freeEntityManager(&(platformer->entityManager));
    deinitCutscene(platformer->gameData.cutscene);
    freeHighScoreSonas(&platformer->highScores, &platformer->sonas);
    heap_caps_free(platformer);
}

/**
 * @brief This callback function is called when an item is selected from the menu
 *
 * @param label The item that was selected from the menu
 * @param selected True if the item was selected with the A button, false if this is a multi-item which scrolled to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static bool mgMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == mgMenuNewGame)
        {
            /*uint16_t levelIndex = getLevelIndex(platformer->gameData.world, platformer->gameData.level);
            if ((levelIndex >= NUM_LEVELS)
                || (!platformer->gameData.debugMode && levelIndex > platformer->unlockables.maxLevelIndexUnlocked))
            {
                soundPlaySfx(&(platformer->soundManager.sndMenuDeny), BZR_STEREO);
                return;
            }*/

            /*if(self->menuSelection == 0){
                self->gameData.world = 1;
                self->gameData.level = 1;
            }*/

            mg_initializeGameDataFromTitleScreen(&(platformer->gameData));

            // change this back to 10 (intro level) before shipping
            platformer->gameData.level = 10;
            mg_loadWsgSet(&(platformer->wsgManager), leveldef[platformer->gameData.level].defaultWsgSetIndex);
            mg_loadMapFromFile(&(platformer->tilemap), leveldef[platformer->gameData.level].filename,
                               &platformer->entityManager);

            changeStateGame(platformer);
            // every level starts with a cutscene
            midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
            soundPlayBgm(&platformer->soundManager.currentBgm, BZR_STEREO);
            stageStartCutscene(&platformer->gameData);
            changeStateCutscene(platformer);
        }
        else if (label == mgMenuContinue)
        {
            /*pango->gameData.level       = settingVal;
            pango->gameData.caravanMode = false;
            pa_initializeGameDataFromTitleScreen(&(pango->gameData));
            pa_setDifficultyLevel(&(pango->wsgManager), &(pango->gameData), settingVal);
            pango->entityManager.activeEnemies = 0;
            pa_loadMapFromFile(&(pango->tilemap), "preset.bin");
            pa_generateMaze(&(pango->tilemap));
            pa_placeEnemySpawns(&(pango->tilemap));

            changeStateReadyScreen(pango);
            deinitMenu(pango->menu);*/

            mg_initializeGameDataFromTitleScreen(&(platformer->gameData));

            // if the 10th level (intro level) isn't cleared
            if (!(platformer->unlockables.levelsCleared & (1 << 10)))
            {
                platformer->gameData.level = 10;
                mg_loadWsgSet(&(platformer->wsgManager), leveldef[platformer->gameData.level].defaultWsgSetIndex);
                mg_loadMapFromFile(&(platformer->tilemap), leveldef[platformer->gameData.level].filename,
                                   &platformer->entityManager);

                changeStateGame(platformer);
                // every level starts with a cutscene
                midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
                soundPlayBgm(&platformer->soundManager.currentBgm, BZR_STEREO);
                stageStartCutscene(&platformer->gameData);
                changeStateCutscene(platformer);
            }
            else
            {
                platformer->gameData.continuesUsed = true;
                changeStateLevelSelect(platformer);
            }
        }
        else if (label == mgMenuHighScores)
        {
            changeStateShowHighScores(platformer);
            platformer->gameData.btnState = 0;
            deinitMenu(platformer->menu);
        }
        else if (label == mgMenuResetScores)
        {
            initializePlatformerHighScores(platformer);
            soundPlaySfx(&(platformer->soundManager.sndSquish), MIDI_SFX);
        }
        else if (label == mgMenuConfirm)
        {
            initializePlatformerUnlockables(platformer);
            savePlatformerUnlockables(platformer);
            platformer->gameData.abilities = 0b00000000;
            writeNvs32(MG_abilitiesNVSKey, platformer->gameData.abilities);
            soundPlaySfx(&(platformer->soundManager.sndDie), MIDI_SFX);
            mg_initializeGameDataFromTitleScreen(&(platformer->gameData));
            platformer->gameData.level = 10;
            mg_loadWsgSet(&(platformer->wsgManager), leveldef[platformer->gameData.level].defaultWsgSetIndex);
            mg_loadMapFromFile(&(platformer->tilemap), leveldef[platformer->gameData.level].filename,
                               &platformer->entityManager);

            changeStateGame(platformer);
            // every level starts with a cutscene
            midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
            soundPlayBgm(&platformer->soundManager.currentBgm, BZR_STEREO);
            stageStartCutscene(&platformer->gameData);
            changeStateCutscene(platformer);
        }
        else if (label == mgMenuSaveAndExit)
        {
            savePlatformerHighScores(platformer);
            savePlatformerUnlockables(platformer);
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (label == mgMenuExit)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
        else if (label == mgMenuPlaceholder)
        {
            soundPlaySfx(&(platformer->soundManager.sndMenuDeny), MIDI_SFX);
        }
        else if (label == mgMenuGo)
        {
            mg_initializeGameDataFromTitleScreen(&(platformer->gameData));
            platformer->gameData.customLevel = true;
            changeStateGame(platformer);
        }
        else if (label == mgMenuAbilityUnlockState)
        {
            platformer->unlockables.levelsCleared = settingVal;
        }
        else if (label == mgMenuSetLevelMetadata)
        {
            platformer->gameData.level = settingVal;
        }
        else if (label == str_giveAbilities)
        {
            platformer->gameData.abilities = 0b11111111;
            writeNvs32(MG_abilitiesNVSKey, platformer->gameData.abilities);
        }
    }
    else
    {
        if (label == str_cheatMode)
        {
            writeNvs32(MG_cheatModeNVSKey, settingVal);
        }
    }

    return false;
}

/**
 * @brief TODO
 *
 * @param elapsedUs
 */
void platformerMainLoop(int64_t elapsedUs)
{
    // Check inputs
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        platformer->btnState          = evt.state;
        platformer->gameData.btnState = evt.state;

        if (platformer->update == &mgUpdateMainMenu)
        {
            // Pass button events to the menu
            platformer->menu = menuButton(platformer->menu, evt);
        }
    }

    platformer->update(platformer);

    platformer->prevBtnState          = platformer->btnState;
    platformer->gameData.prevBtnState = platformer->prevBtnState;
}

void changeStateMainMenu(platformer_t* self)
{
    self->gameData.frameCount  = 0;
    self->gameData.changeState = 0;
    self->update               = &mgUpdateMainMenu;
    int32_t outVal             = 0;
    readNvs32(MG_abilitiesNVSKey, &outVal);
    self->gameData.abilities = outVal;
    mgBuildMainMenu(self);
}

void mgBuildMainMenu(platformer_t* self)
{
    // Initialize the menu
    self->menu = initMenu(platformerName, mgMenuCb);

    size_t size;
    if (readNvsBlob("user_level", NULL, &size))
    {
        self->menu = startSubMenu(self->menu, mgMenuPlayCustomLevel);
        addSingleItemToMenu(self->menu, mgMenuGo);
        self->menu = startSubMenu(self->menu, mgMenuSetGameState);
        addSettingsItemToMenu(self->menu, mgMenuSetLevelMetadata, &mgAbilityUnlockStateSettingBounds, 1);
        addSettingsItemToMenu(self->menu, mgMenuAbilityUnlockState, &mgAbilityUnlockStateSettingBounds, 255);
        self->menu = endSubMenu(self->menu);
        self->menu = endSubMenu(self->menu);
    }

    if (self->unlockables.levelsCleared || self->gameData.abilities)
    {
        addSingleItemToMenu(self->menu, mgMenuContinue);

        self->menu = startSubMenu(self->menu, mgMenuStartOver);
        addSingleItemToMenu(self->menu, mgMenuConfirm);
        self->menu = endSubMenu(self->menu);
    }
    else
    {
        addSingleItemToMenu(self->menu, mgMenuNewGame);
    }

    addSingleItemToMenu(self->menu, mgMenuHighScores);

    self->menu           = startSubMenu(self->menu, mgMenuOptions);
    settingParam_t sp_tf = {
        .min = trueFalseVals[0],
        .max = trueFalseVals[ARRAY_SIZE(trueFalseVals) - 1],
        .def = trueFalseVals[0],
    };
    int32_t cheatMode = 0;
    if (readNvs32(MG_cheatModeNVSKey, &cheatMode) == false)
    {
        writeNvs32(MG_cheatModeNVSKey, 0);
    }
    addSettingsOptionsItemToMenu(self->menu, str_cheatMode, strs_on_off, trueFalseVals, ARRAY_SIZE(strs_on_off), &sp_tf,
                                 cheatMode);
    // We're not shipping that button
    // addSingleItemToMenu(self->menu, str_giveAbilities);
    self->menu = endSubMenu(self->menu);

    if (self->gameData.debugMode)
    {
        addSingleItemToMenu(self->menu, mgMenuResetProgress);
        addSingleItemToMenu(self->menu, mgMenuResetScores);
        addSingleItemToMenu(self->menu, mgMenuSaveAndExit);
    }
    else
    {
        addSingleItemToMenu(self->menu, mgMenuExit);
    }
}

void mgUpdateMainMenu(platformer_t* self)
{
    // Draw the menu
    drawMenuMega(self->menu, self->menuRenderer, 16666);
}

void updateGame(platformer_t* self)
{
    // Clear the display
    // fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    mg_updateEntities(&(self->entityManager));

    mg_drawTileMap(&(self->tilemap));
    // JVeg wants the hud drawn behind entities.
    drawPlatformerHud(&(self->font), &(self->gameData));
    mg_drawEntities(&(self->entityManager));
    mg_updateLeds(&self->entityManager);
    detectGameStateChange(self);
    detectBgmChange(self);

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 59)
    {
        self->gameData.frameCount = 0;
        if (!self->gameData.pauseCountdown)
        {
            self->gameData.countdown--;
        }
        self->gameData.inGameTimer++;

        if (self->gameData.countdown < 10)
        {
            midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
            soundPlayBgm(&(self->soundManager.sndOuttaTime), BZR_STEREO);
        }

        if (self->gameData.countdown < 0)
        {
            killPlayer(self->entityManager.playerEntity);
        }
    }

    updateComboTimer(&(self->gameData));
}

void drawPlatformerHud(font_t* font, mgGameData_t* gameData)
{
    // char coinStr[8];
    // snprintf(coinStr, sizeof(coinStr) - 1, "C:%02d", gameData->coins);

    char scoreStr[32];
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%06" PRIu32, gameData->score);

    // char levelStr[15];
    // snprintf(levelStr, sizeof(levelStr) - 1, "Level %d-%d", gameData->world, gameData->level);

    char livesStr[8];
    snprintf(livesStr, sizeof(livesStr) - 1, "x%02d", gameData->lives);

    char timeStr[10];
    snprintf(timeStr, sizeof(timeStr) - 1, "T:%03d", gameData->countdown);

    /*if (gameData->frameCount > 29)
    {
        drawText(font, c500, "1UP", 24, 2);
    }*/

    // drawText(font, c555, coinStr, 160, 16);
    drawText(font, c000, scoreStr, 34, 4);
    drawText(font, c555, scoreStr, 32, 2);
    // drawText(font, c555, levelStr, 152, 2);
    drawText(font, c000, timeStr, 214, 4);
    drawText(font, (gameData->countdown > 30) ? c555 : redColors[(gameData->frameCount >> 3) % 4], timeStr, 212, 2);

    if (platformer->entityManager.playerEntity != NULL)
    {
        drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_BOTTOM_ALPHA], 8, MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION);

        int8_t originalHp = platformer->entityManager.playerEntity->hp;
        if (originalHp < 0)
        {
            originalHp = 0;
        }
        int8_t hp  = originalHp;
        bool isOdd = hp % 2;

        if (hp > 60)
        {
            hp = 60;
        }

        int16_t draw_y; // y location used to draw health tiles.
        int16_t pip_y;  // y location used to draw half pips of health (at the top).
        for (uint8_t i = 0; i < 4; i++)
        {
            draw_y = MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION - 16 - (i * 16);
            if (hp > 12)
            {
                drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_MIDDLE_6], 8, draw_y);
            }
            else
            {
                drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_MIDDLE_0 + hp / 2], 8, draw_y);
                if (isOdd && originalHp > i * 12 && originalHp < (i + 1) * 12)
                {
                    pip_y = draw_y + 15 - ((hp / 2) % 6) * 2;
                    drawLineFast(14, pip_y, 17, pip_y, c133);
                    drawLineFast(15, pip_y, 16, pip_y, c143);
                }
            }

            hp -= 12;

            if (hp < 0)
            {
                hp = 0;
            }
        }

        draw_y = MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION - 80; // same as MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION - 16 - (4*16);
        if (hp == 12)
        {
            drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_TOP_6], 8, draw_y);
        }
        else
        {
            drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_TOP_0 + hp / 2], 8, draw_y);
            if (isOdd && originalHp > 48) // same as > 4 * 12
            {
                pip_y = draw_y + 15 - ((hp / 2) % 6) * 2;
                drawLineFast(14, pip_y, 17, pip_y, c133);
                drawLineFast(15, pip_y, 16, pip_y, c143);
            }
        }

        if (platformer->gameData.abilities & (1U << MG_CAN_OF_SALSA_ABILITY))
        {
            draw_y = MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION
                     - 97; // same as MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION - 1 - (6 * 16);
            if (originalHp < 61)
            {
                drawWsgSimple(&platformer->wsgManager.wsgs[MG_WSG_HP_CAN_OF_SALSA_0], 4, draw_y);
            }
            else
            {
                drawWsgSimple(&platformer->wsgManager.wsgs[MG_WSG_HP_CAN_OF_SALSA_0 + (originalHp - 60) / 2], 4,
                              draw_y);
                if (isOdd && originalHp > 60)
                {
                    pip_y = draw_y + 13 - ((originalHp / 2) % 6) * 2;
                    drawLineFast(14, pip_y, 17, pip_y, c133);
                    drawLineFast(15, pip_y, 16, pip_y, c143);
                }
            }
        }
    }

    if (platformer->entityManager.bossEntity != NULL)
    {
        drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_BOTTOM_BIGMA], 256, MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION);

        int8_t originalHp = platformer->entityManager.bossEntity->hp;
        if (originalHp < 0)
        {
            originalHp = 0;
        }
        int8_t hp  = originalHp;
        bool isOdd = hp % 2;

        if (hp > 60)
        {
            hp = 60;
        }

        int16_t draw_y; // y location used to draw health tiles.
        int16_t pip_y;  // y location used to draw half pips of health (at the top).
        for (uint8_t i = 0; i < 4; i++)
        {
            draw_y = MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION - 16 - (i * 16);
            if (hp > 12)
            {
                drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_BOSS_MIDDLE_6], 256, draw_y);
            }
            else
            {
                drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_BOSS_MIDDLE_0 + hp / 2], 256, draw_y);
                if (isOdd && originalHp > i * 12 && originalHp < (i + 1) * 12)
                {
                    pip_y = draw_y + 15 - ((hp / 2) % 6) * 2;
                    drawLineFast(262, pip_y, 265, pip_y, c321);
                    drawLineFast(263, pip_y, 264, pip_y, c441);
                }
            }

            hp -= 12;

            if (hp < 0)
            {
                hp = 0;
            }
        }

        draw_y = MG_PLAYER_LIFEBAR_Y_BOTTOM_LOCATION - 16 - (4 * 16);
        if (hp == 12)
        {
            drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_BOSS_TOP_6], 256, draw_y);
        }
        else
        {
            drawWsgTile(&platformer->wsgManager.wsgs[MG_WSG_HP_BOSS_TOP_0 + hp / 2], 256, draw_y);
            if (isOdd && originalHp > 4 * 12)
            {
                pip_y = draw_y + 15 - ((hp / 2) % 6) * 2;
                drawLineFast(262, pip_y, 265, pip_y, c321);
                drawLineFast(263, pip_y, 264, pip_y, c441);
            }
        }
    }

    drawText(font, c000, livesStr, 6, 166);
    drawText(font, c555, livesStr, 4, 164);

    if (gameData->comboTimer == 0)
    {
        return;
    }

    snprintf(scoreStr, sizeof(scoreStr) - 1, "+%" PRIu32 " (x%d)", gameData->comboScore, gameData->combo);
    drawText(font, (gameData->comboTimer < 60) ? c030 : greenColors[(platformer->gameData.frameCount >> 3) % 4],
             scoreStr, 32, 16);
}

void updateTitleScreen(platformer_t* self)
{
    // Clear the display
    // fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    self->gameData.frameCount++;

    if ((self->gameData.btnState & cheatCode[self->menuSelection])
        && !(self->gameData.prevBtnState & cheatCode[self->menuSelection]))
    {
        self->menuSelection++;

        if (self->menuSelection > 8)
        {
            self->menuSelection      = 0;
            self->menuState          = 1;
            self->gameData.debugMode = true;
            mg_setBgm(&self->soundManager, MG_BGM_LEVEL_CLEAR_JINGLE);
            midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
            soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
            globalMidiPlayerGet(MIDI_BGM)->loop = false;
        }
    }

    if ((((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A))
         || ((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START))))
    {
        self->gameData.btnState = 0;
        self->menuSelection     = 0;

        if (!self->gameData.debugMode)
        {
            soundPlaySfx(&(self->soundManager.sndMenuConfirm), BZR_STEREO);
        }

        changeStateMainMenu(self);
        return;
    }

    drawPlatformerTitleScreen(&(self->font), &(self->gameData));

    // leds
}

void drawPlatformerTitleScreen(font_t* font, mgGameData_t* gameData)
{
    mg_drawTileMap(&(platformer->tilemap));

    drawText(font, c555, "Mega Pulse EX", 40, 32);

    if (platformer->gameData.debugMode)
    {
        drawText(font, c555, "Debug Mode", 80, 48);
    }

    switch (platformer->menuState)
    {
        case 0:
        {
            if ((gameData->frameCount % 60) < 30)
            {
                drawText(font, c555, "- Press START button -", 20, 128);
            }
            break;
        }

        case 1:
        {
            drawText(font, c555, "Start Game", 48, 128);

            /*if (platformer->gameData.debugMode || platformer->unlockables.levelsCleared > 0)
            {
                char levelStr[24];
                snprintf(levelStr, sizeof(levelStr) - 1, "Level Select: %d-%d", gameData->world, gameData->level);
                drawText(font, c555, levelStr, 48, 144);
            }*/

            if (platformer->gameData.debugMode)
            {
                drawText(font, c555, "Reset Progress", 48, 160);
                drawText(font, c555, "Reset High Scores", 48, 176);
                drawText(font, c555, "Save & Exit to Menu", 48, 192);
            }
            else
            {
                drawText(font, c555, "High Scores", 48, 160);
                drawText(font, c555, "Achievements", 48, 176);
                drawText(font, c555, "Exit to Menu", 48, 192);
            }

            drawText(font, c555, "->", 32, 128 + platformer->menuSelection * 16);

            break;
        }

        case 2:
        {
            if (platformer->unlockables.gameCleared)
            {
                drawText(font, redColors[(gameData->frameCount >> 3) % 4], "Beat the game!", 48, 80);
            }

            if (platformer->unlockables.oneCreditCleared)
            {
                drawText(font, yellowColors[(gameData->frameCount >> 3) % 4], "1 Credit Clear!", 48, 96);
            }

            if (platformer->unlockables.bigScore)
            {
                drawText(font, greenColors[(gameData->frameCount >> 3) % 4], "Got 4 million points!", 48, 112);
            }

            if (platformer->unlockables.biggerScore)
            {
                drawText(font, cyanColors[(gameData->frameCount >> 3) % 4], "Got 10 million points!", 48, 128);
            }

            if (platformer->unlockables.fastTime)
            {
                drawText(font, purpleColors[(gameData->frameCount >> 3) % 4], "Beat within 25 min!", 48, 144);
            }

            if (platformer->unlockables.gameCleared && platformer->unlockables.oneCreditCleared
                && platformer->unlockables.bigScore && platformer->unlockables.biggerScore
                && platformer->unlockables.fastTime)
            {
                drawText(font, rgbColors[(gameData->frameCount >> 3) % 4], "100% 100% 100%", 48, 160);
            }

            drawText(font, c555, "Press B to Return", 48, 192);
            break;
        }

        default:
            break;
    }
}

void changeStateReadyScreen(platformer_t* self)
{
    self->gameData.frameCount = 0;

    globalMidiPlayerGet(MIDI_BGM)->loop = false;
    // soundPlayBgm(&(self->soundManager.bgmIntro), BZR_STEREO);

    mg_resetGameDataLeds(&(self->gameData));

    self->update = &updateReadyScreen;
}

void updateCutsceneScreen(platformer_t* self)
{
    updateCutscene(self->gameData.cutscene, self->btnState);
    drawCutsceneScreen(self);
}

void drawCutsceneScreen(platformer_t* self)
{
    mg_drawTileMap(&(self->tilemap));
    mg_drawEntities(&(self->entityManager));
    drawCutscene(self->gameData.cutscene, &self->font);
}

void updateReadyScreen(platformer_t* self)
{
    // Clear the display
    // fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 60) // 179)
    {
        if (globalMidiPlayerGet(MIDI_BGM)->paused)
        {
            midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
            soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
        }
        self->update = &updateGame;
    }

    mg_drawTileMap(&(self->tilemap));
    mg_drawEntities(&(self->entityManager));
    drawReadyScreen(&(self->font), &(self->gameData));
}

void drawReadyScreen(font_t* font, mgGameData_t* gameData)
{
    // drawPlatformerHud(font, gameData);
    int16_t xOff = (TFT_WIDTH - textWidth(font, str_get_ready)) / 2;

    if (gameData->frameCount & 0b1111)
    {
        drawText(font, c000, ((gameData->levelDeaths > 0) ? str_get_ready_2 : str_get_ready), xOff + 2, 130);
        drawText(font, c555, ((gameData->levelDeaths > 0) ? str_get_ready_2 : str_get_ready), xOff, 128);
    }

    /*if (getLevelIndex(gameData->world, gameData->level) == 0)
    {
        drawText(font, c555, "A: Jump", xOff, 128 + (font->height + 3) * 3);
        drawText(font, c555, "B: Run / Fire", xOff, 128 + (font->height + 3) * 4);
    }*/
}

void changeStateGame(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.currentBgm = 0;
    mg_resetGameDataLeds(&(self->gameData));

    mg_deactivateAllEntities(&(self->entityManager), false);

    bool checkpointActive = self->gameData.checkpointLevel && self->gameData.checkpointSpawnIndex;
    if (checkpointActive)
    {
        self->gameData.level = self->gameData.checkpointLevel;
    }

    uint16_t levelIndex = getLevelIndex(1, self->gameData.level) + 1;

    mg_loadWsgSet(&(platformer->wsgManager), leveldef[levelIndex].defaultWsgSetIndex);

    if (self->gameData.customLevel)
    {
        mg_loadMapFromFile(&(platformer->tilemap), -69, &platformer->entityManager);
    }
    else
    {
        mg_loadMapFromFile(&(platformer->tilemap), leveldef[levelIndex].filename, &platformer->entityManager);
    }
    self->gameData.countdown      = leveldef[levelIndex].timeLimit;
    self->gameData.pauseCountdown = false;

    mgEntityManager_t* entityManager = &(self->entityManager);

    mgEntitySpawnData_t* playerSpawn = (checkpointActive)
                                           ? &(self->tilemap.entitySpawns[self->gameData.checkpointSpawnIndex])
                                           : self->tilemap.defaultPlayerSpawn;

    if (playerSpawn != NULL)
    {
        entityManager->viewEntity = mg_createPlayer(entityManager, playerSpawn->tx * 16 + playerSpawn->xOffsetInPixels,
                                                    playerSpawn->ty * 16 + playerSpawn->yOffsetInPixels);
        entityManager->playerEntity = entityManager->viewEntity;
        // entityManager->playerEntity->hp = self->gameData.initialHp;
        mg_viewFollowEntity(&(self->tilemap), entityManager->playerEntity);
    }

    // Tiled wouldn't let me export with a power-up so I'm doing this here to top off can of salsa at the start of boss
    // rush and hank waddle
    if (self->gameData.level == 11 || self->gameData.level == 21)
    {
        // Creates a power up just right of the player spawn
        mg_createEntity(entityManager, ENTITY_POWERUP, playerSpawn->tx * 16 + playerSpawn->xOffsetInPixels + 50,
                        playerSpawn->ty * 16 + playerSpawn->yOffsetInPixels + 8);
    }

    entityManager->bossEntity = NULL;

    // mg_updateLedsHpMeter(&(self->entityManager), &(self->gameData));

    self->tilemap.executeTileSpawnAll = true;

    // self->gameData.changeBgm = MG_BGM_KINETIC_DONUT;
    mg_setBgm(&self->soundManager, leveldef[self->gameData.level].mainBgmIndex);
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
    self->gameData.bgColors = leveldef[self->gameData.level].bgColors;

    soundStop(true);

    self->update = &updateReadyScreen;
}

void changeStateCutscene(platformer_t* self)
{
    self->update = &updateCutsceneScreen;
}

static void mg_backgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, platformer->gameData.bgColors[((y >> 6) % 4)]);
}

void detectGameStateChange(platformer_t* self)
{
    if (!self->gameData.changeState)
    {
        return;
    }

    switch (self->gameData.changeState)
    {
        case MG_ST_DEAD:
            changeStateDead(self);
            break;

        case MG_ST_READY_SCREEN:
            changeStateGame(self);
            break;

        case MG_ST_LEVEL_CLEAR:
            changeStateLevelClear(self);
            break;

        case MG_ST_PAUSE:
            changeStatePause(self);
            break;

        case MG_ST_CUTSCENE:
            changeStateCutscene(self);
            break;

        default:
            break;
    }

    self->gameData.changeState = 0;
}

void detectBgmChange(platformer_t* self)
{
    if (self->gameData.changeBgm == MG_BGM_NO_CHANGE)
    {
        return;
    }

    if (mg_setBgm(&(self->soundManager), self->gameData.changeBgm))
    {
        midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
        soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
    }

    self->gameData.currentBgm = self->gameData.changeBgm;
    self->gameData.changeBgm  = MG_BGM_NO_CHANGE;
}

void changeStateDead(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.lives--;
    self->gameData.levelDeaths++;
    self->gameData.combo      = 0;
    self->gameData.comboTimer = 0;
    self->gameData.initialHp  = 1;

    soundStop(true);
    mg_resetGameDataLeds(&self->gameData);
    globalMidiPlayerGet(MIDI_BGM)->loop = false;
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    soundPlayBgm(&(self->soundManager.sndDie), BZR_STEREO);
    self->entityManager.viewEntity = NULL;

    self->update = &updateDead;
}

void updateDead(platformer_t* self)
{
    self->gameData.frameCount++;
    if (self->gameData.frameCount > 200)
    {
        if (self->gameData.lives > 0)
        {
            changeStateGame(self);
        }
        else
        {
            changeStateGameOver(self);
        }
    }

    mg_updateEntities(&(self->entityManager));
    mg_drawTileMap(&(self->tilemap));
    mg_drawEntities(&(self->entityManager));
    drawPlatformerHud(&(self->font), &(self->gameData));
    mg_updateLedsDead(&self->gameData);

    if (self->gameData.countdown < 0)
    {
        drawText(&(self->font), c555, str_time_up, (TFT_WIDTH - textWidth(&(self->font), str_time_up)) / 2, 128);
    }
}

void updateGameOver(platformer_t* self)
{
    // Clear the display
    // fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;
    if (self->gameData.frameCount > 179)
    {
        // Handle unlockables
        if (!self->gameData.debugMode)
        {
            savePlatformerUnlockables(self);
        }

        changeStateNameEntry(self);
    } else if (self->gameData.frameCount == 60)
    {
        if (!self->gameData.cheatMode && !self->gameData.debugMode && self->gameData.score >= BIG_SCORE)
        {
            trophyUpdate(&platformerTrophies[11], 1, true);
        }
    } else if (self->gameData.frameCount == 120)
    {
        if (!self->gameData.cheatMode && !self->gameData.debugMode && self->gameData.score >= BIGGER_SCORE)
        {
            trophyUpdate(&platformerTrophies[12], 1, true);
        }
    }

    drawGameOver(&(self->font), &(self->gameData));
    mg_updateLedsGameOver(&(self->gameData));
}

void changeStateGameOver(platformer_t* self)
{
    self->gameData.frameCount = 0;
    mg_resetGameDataLeds(&(self->gameData));
    globalMidiPlayerGet(MIDI_BGM)->loop = false;
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    soundPlayBgm(&(self->soundManager.bgmGameOver), BZR_STEREO);
    self->update = &updateGameOver;
}

void drawGameOver(font_t* font, mgGameData_t* gameData)
{
    drawPlatformerHud(font, gameData);
    drawText(font, c555, str_game_over, (TFT_WIDTH - textWidth(font, str_game_over)) / 2, 128);
}

void changeStateTitleScreen(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->gameData.gameState  = MG_ST_TITLE_SCREEN;
    self->update              = &updateTitleScreen;
}

void changeStateLevelClear(platformer_t* self)
{
    self->gameData.frameCount           = 0;
    self->gameData.checkpointLevel      = 0;
    self->gameData.checkpointSpawnIndex = 0;
    self->gameData.levelDeaths          = 0;
    self->gameData.initialHp            = self->entityManager.playerEntity->hp;
    self->gameData.extraLifeCollected   = false;
    self->gameData.enemiesKilled        = 0;
    mg_resetGameDataLeds(&(self->gameData));
    self->update = &updateLevelClear;
}

void updateLevelClear(platformer_t* self)
{
    // Clear the display
    // fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, self->gameData.bgColor);

    self->gameData.frameCount++;

    if (self->gameData.frameCount > 100)
    {
        if (self->gameData.countdown > 0)
        {
            self->gameData.countdown--;

            if (self->gameData.countdown % 2)
            {
                globalMidiPlayerGet(MIDI_SFX)->loop = false;
                soundPlaySfx(&(self->soundManager.sndTally), BZR_STEREO);
            }

            uint16_t comboPoints = 50 * self->gameData.combo;

            self->gameData.score += comboPoints;
            self->gameData.comboScore = comboPoints;

            if (self->gameData.combo > 1)
            {
                self->gameData.combo--;
            }
        }
        else if (self->gameData.frameCount % 60 == 0)
        {
            // Hey look, it's a frame rule!

            // uint16_t levelIndex = getLevelIndex(1, self->gameData.level);

            if (false /*levelIndex >= NUM_LEVELS - 1*/)
            {
                // Game Cleared!

                // if (!self->gameData.debugMode)
                // {
                //     // Determine achievements
                //     self->unlockables.gameCleared = true;

                //     if (!self->gameData.continuesUsed)
                //     {
                //         self->unlockables.oneCreditCleared = true;

                //         if (self->gameData.inGameTimer < FAST_TIME)
                //         {
                //             self->unlockables.fastTime = true;
                //         }
                //     }

                //     if (self->gameData.score >= BIG_SCORE)
                //     {
                //         self->unlockables.bigScore = true;
                //     }

                //     if (self->gameData.score >= BIGGER_SCORE)
                //     {
                //         self->unlockables.biggerScore = true;
                //     }
                // }

                changeStateGameClear(self);
            }
            else if (globalMidiPlayerGet(MIDI_BGM)->paused) // also checks the level clear jingle has finished.
            {
                // Advance to the next level
                /*self->gameData.level++;
                if (self->gameData.level > 4)
                {
                    self->gameData.world++;
                    self->gameData.level = 1;
                }

                // Unlock the next level
                levelIndex++;
                if (levelIndex > self->unlockables.maxLevelIndexUnlocked)
                {
                    self->unlockables.maxLevelIndexUnlocked = levelIndex;
                }*/

                self->unlockables.levelsCleared |= (1 << self->gameData.level);

                if(self->unlockables.levelsCleared == 0b1111111111110)
                {
                    //Beat the game!
                    changeStateGameClear(self);
                    return;
                }

                changeStateLevelSelect(self);
            }

            if (!self->gameData.debugMode)
            {
                savePlatformerUnlockables(self);
            }
        }
    }

    mg_updateEntities(&(self->entityManager));
    mg_drawTileMap(&(self->tilemap));
    mg_drawEntities(&(self->entityManager));
    drawPlatformerHud(&(self->font), &(self->gameData));
    drawLevelClear(&(self->font), &(self->gameData));
    mg_updateLedsLevelClear(&(self->gameData));
}

void drawLevelClear(font_t* font, mgGameData_t* gameData)
{
    drawPlatformerHud(font, gameData);
    drawText(font, c555, str_well_done, (TFT_WIDTH - textWidth(font, str_well_done)) / 2, 128);
}

void changeStateGameClear(platformer_t* self)
{
    self->gameData.frameCount = 0;
    self->update              = &updateGameClear;
    mg_resetGameDataLeds(&(self->gameData));
    mg_setBgm(&(self->soundManager), 0);
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    //soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
}

void updateGameClear(platformer_t* self)
{
    self->gameData.frameCount++;

    if (self->gameData.frameCount > 120)
    {
        if (self->gameData.lives > 0)
        {
            if (self->gameData.frameCount % 60 == 0)
            {
                self->gameData.lives--;
                self->gameData.score += 200000;
                soundPlaySfx(&(self->soundManager.snd1up), BZR_STEREO);
            }
        }
        else if (self->gameData.frameCount % 600 == 0)
        {
            changeStateGameOver(self);
        }
    }

    if(!self->gameData.cheatMode && !self->gameData.debugMode)
    {
        if(self->gameData.frameCount == 15 && !self->gameData.continuesUsed)
        {
            trophyUpdate(&platformerTrophies[13], 1, true);
        }
        
        if(self->gameData.frameCount == 45 && !self->gameData.continuesUsed && self->gameData.inGameTimer < FAST_TIME)
        {
            trophyUpdate(&platformerTrophies[14], 1, true);
        }
    }

    drawPlatformerHud(&(self->font), &(self->gameData));
    drawGameClear(&(self->font), &(self->gameData));
    mg_updateLedsGameClear(&(self->gameData));
}

void drawGameClear(font_t* font, mgGameData_t* gameData)
{
    drawPlatformerHud(font, gameData);

    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr) - 1, "Clear Time: %06" PRIu32 "s", gameData->inGameTimer);

    drawText(font, yellowColors[(gameData->frameCount >> 3) % 4], str_congrats,
             (TFT_WIDTH - textWidth(font, str_congrats)) / 2, 48);

    // if (gameData->frameCount > 120)
    // {
    //     drawText(font, c555, "You've completed your", 8, 80);
    //     drawText(font, c555, "trip across Swadge Land", 8, 96);
    // }

    if (gameData->frameCount > 15)
    {
        drawText(font, (gameData->inGameTimer < FAST_TIME) ? cyanColors[(gameData->frameCount >> 3) % 4] : c555,
                 timeStr, (TFT_WIDTH - textWidth(font, timeStr)) / 2, 80);
    }

    // if (gameData->frameCount > 300)
    // {
    //     drawText(font, c555, "The Swadge staff", 8, 144);
    //     drawText(font, c555, "thanks you for playing!", 8, 160);
    // }

    if (gameData->frameCount > 60)
    {
        drawText(font, (gameData->lives > 0) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555,
                 "Bonus 200000pts per life!", (TFT_WIDTH - textWidth(font, "Bonus 100000pts per life!")) / 2, 112);
    }

    /*
    drawText(font, c555, "Thanks for playing.", 24, 48);
    drawText(font, c555, "Many more battle scenes", 8, 96);
    drawText(font, c555, "will soon be available!", 8, 112);
    drawText(font, c555, "Bonus 100000pts per life!", 8, 160);
    */
}

void initializePlatformerHighScores(platformer_t* self)
{
    // self->highScores.scores[0] = 100000;
    // self->highScores.scores[1] = 80000;
    // self->highScores.scores[2] = 40000;
    // self->highScores.scores[3] = 20000;
    // self->highScores.scores[4] = 10000;

    // for (uint8_t i = 0; i < NUM_PLATFORMER_HIGH_SCORES; i++)
    // {
    //     self->highScores.initials[i][0] = 'J' + i;
    //     self->highScores.initials[i][1] = 'P' - i;
    //     self->highScores.initials[i][2] = 'V' + i;
    // }
}

void loadPlatformerHighScores(platformer_t* self)
{
    size_t size = sizeof(platformerHighScores_t);
    // Try reading the value
    if (false == readNvsBlob(KEY_SCORES, &(self->highScores), &(size)))
    {
        // Value didn't exist, so write the default
        initializePlatformerHighScores(self);
    }
}

void savePlatformerHighScores(platformer_t* self)
{
    size_t size = sizeof(platformerHighScores_t);
    writeNvsBlob(KEY_SCORES, &(self->highScores), size);
}

void initializePlatformerUnlockables(platformer_t* self)
{
    self->unlockables.levelsCleared    = 0;
    self->unlockables.gameCleared      = false;
    self->unlockables.oneCreditCleared = false;
    self->unlockables.bigScore         = false;
    self->unlockables.fastTime         = false;
    self->unlockables.biggerScore      = false;
}

void loadPlatformerUnlockables(platformer_t* self)
{
    size_t size = sizeof(platformerUnlockables_t);
    // Try reading the value
    if (false == readNvsBlob(KEY_UNLOCKS, &(self->unlockables), &(size)))
    {
        // Value didn't exist, so write the default
        initializePlatformerUnlockables(self);
    }

    // self->unlockables.levelsCleared = 0b0111011110;
}

void savePlatformerUnlockables(platformer_t* self)
{
    size_t size = sizeof(platformerUnlockables_t);
    writeNvsBlob(KEY_UNLOCKS, &(self->unlockables), size);
}

void drawPlatformerHighScores(font_t* font, highScores_t* highScores, swadgesona_t* sonas, mgGameData_t* gameData)
{
    drawText(font, c555, "RANK   SCORE  NAME", 8, 80);
    for (uint8_t i = 0; i < NUM_PLATFORMER_HIGH_SCORES; i++)
    {
        char rowStr[64];
        snprintf(rowStr, sizeof(rowStr) - 1, "%d%8.6" PRIu32 " %s", i + 1, (uint32_t) highScores->highScores[i].score, sonas[i].name.nameBuffer);
        drawText(font, (gameData->rank == i) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555, rowStr,
                 32, 96 + i * 24);
        drawWsgSimpleHalf(&(sonas[i].image), 4, 86 + i *24);
    }
}

uint8_t getHighScoreRank(platformerHighScores_t* highScores, uint32_t newScore)
{
    uint8_t i;
    for (i = 0; i < NUM_PLATFORMER_HIGH_SCORES; i++)
    {
        if (highScores->scores[i] < newScore)
        {
            break;
        }
    }

    return i;
}

void insertScoreIntoHighScores(platformerHighScores_t* highScores, uint32_t newScore, char newInitials[], uint8_t rank)
{
    if (rank >= NUM_PLATFORMER_HIGH_SCORES)
    {
        return;
    }

    for (uint8_t i = NUM_PLATFORMER_HIGH_SCORES - 1; i > rank; i--)
    {
        highScores->scores[i]      = highScores->scores[i - 1];
        highScores->initials[i][0] = highScores->initials[i - 1][0];
        highScores->initials[i][1] = highScores->initials[i - 1][1];
        highScores->initials[i][2] = highScores->initials[i - 1][2];
    }

    highScores->scores[rank]      = newScore;
    highScores->initials[rank][0] = newInitials[0];
    highScores->initials[rank][1] = newInitials[1];
    highScores->initials[rank][2] = newInitials[2];
}

void changeStateNameEntry(platformer_t* self)
{
   /* self->gameData.frameCount = 0;
    uint8_t rank              = getHighScoreRank(&(self->highScores), self->gameData.score);
    self->gameData.rank       = rank;
    self->menuState           = 0;
    self->menuSelection       = 0;

    mg_resetGameDataLeds(&(self->gameData));

    if (rank >= NUM_PLATFORMER_HIGH_SCORES || self->gameData.debugMode)
    {
        self->menuSelection = 0;
        self->gameData.rank = NUM_PLATFORMER_HIGH_SCORES;
        changeStateShowHighScores(self);
        return;
    }

    mg_setBgm(&(self->soundManager), MG_BGM_NAME_ENTRY);
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
    self->menuSelection = self->gameData.initials[0];
    self->update        = &updateNameEntry;*/

    //Bypass for Swadgepass Scoring

    self->menuSelection = 0;
    if (self->gameData.cheatMode || self->gameData.debugMode)
    {
        changeStateShowHighScores(self);
        return;
    }

    score_t scores[]         = {{.score = self->gameData.score, .spKey = {0}, .swadgesona = {0}}};
    updateHighScores(&self->highScores, KEY_SCORES, scores, 1);
    changeStateShowHighScores(self);
}

void updateNameEntry(platformer_t* self)
{
    // fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;

    if (self->gameData.btnState & PB_LEFT && !(self->gameData.prevBtnState & PB_LEFT))
    {
        self->menuSelection--;

        if (self->menuSelection < 32)
        {
            self->menuSelection = 90;
        }

        self->gameData.initials[self->menuState] = self->menuSelection;
        soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
    }
    else if (self->gameData.btnState & PB_RIGHT && !(self->gameData.prevBtnState & PB_RIGHT))
    {
        self->menuSelection++;

        if (self->menuSelection > 90)
        {
            self->menuSelection = 32;
        }

        self->gameData.initials[self->menuState] = self->menuSelection;
        soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
    }
    else if (self->gameData.btnState & PB_B && !(self->gameData.prevBtnState & PB_B))
    {
        if (self->menuState > 0)
        {
            self->menuState--;
            self->menuSelection = self->gameData.initials[self->menuState];
            soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
        }
        else
        {
            soundPlaySfx(&(self->soundManager.sndMenuDeny), BZR_STEREO);
        }
    }
    else if (self->gameData.btnState & PB_A && !(self->gameData.prevBtnState & PB_A))
    {
        self->menuState++;

        if (self->menuState > 2)
        {
            insertScoreIntoHighScores(&(self->highScores), self->gameData.score, self->gameData.initials,
                                      self->gameData.rank);
            savePlatformerHighScores(self);
            changeStateShowHighScores(self);
            soundPlaySfx(&(self->soundManager.sndPowerUp), BZR_STEREO);
        }
        else
        {
            self->menuSelection = self->gameData.initials[self->menuState];
            soundPlaySfx(&(self->soundManager.sndMenuSelect), BZR_STEREO);
        }
    }

    drawNameEntry(&(self->font), &(self->gameData), self->menuState);
    mg_updateLedsShowHighScores(&(self->gameData));
}

void drawNameEntry(font_t* font, mgGameData_t* gameData, uint8_t currentInitial)
{
    drawText(font, greenColors[(platformer->gameData.frameCount >> 3) % 4], str_initials,
             (TFT_WIDTH - textWidth(font, str_initials)) / 2, 64);

    char rowStr[32];
    snprintf(rowStr, sizeof(rowStr) - 1, "%d   %06" PRIu32, gameData->rank + 1, gameData->score);
    drawText(font, c555, rowStr, 64, 128);

    for (uint8_t i = 0; i < 3; i++)
    {
        snprintf(rowStr, sizeof(rowStr) - 1, "%c", gameData->initials[i]);
        drawText(font, (currentInitial == i) ? highScoreNewEntryColors[(gameData->frameCount >> 3) % 4] : c555, rowStr,
                 192 + 16 * i, 128);
    }
}

void changeStateShowHighScores(platformer_t* self)
{
    initHighScoreSonas(&self->highScores, &self->sonas);
    
    self->gameData.frameCount = 0;
    self->update              = &updateShowHighScores;
}

void updateShowHighScores(platformer_t* self)
{
    // fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

    self->gameData.frameCount++;

    if ((self->gameData.frameCount > 300)
        || (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START))
            || ((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A))))
    {
        self->menuState     = 0;
        self->menuSelection = 0;
        soundStop(true);
        changeStateMainMenu(self);
    }

    drawShowHighScores(&(self->font), self->menuState);
    drawPlatformerHighScores(&(self->font), &(self->highScores), &(self->sonas), &(self->gameData));

    mg_updateLedsShowHighScores(&(self->gameData));
}

void drawShowHighScores(font_t* font, uint8_t menuState)
{
    if (platformer->easterEgg)
    {
        drawText(font, highScoreNewEntryColors[(platformer->gameData.frameCount >> 3) % 4], str_hbd,
                 (TFT_WIDTH - textWidth(font, str_hbd)) / 2, 32);
    }
    else if (menuState == 3)
    {
        drawText(font, redColors[(platformer->gameData.frameCount >> 3) % 4], str_registrated,
                 (TFT_WIDTH - textWidth(font, str_registrated)) / 2, 32);
    }
    else
    {
        drawText(font, c555, str_do_your_best, (TFT_WIDTH - textWidth(font, str_do_your_best)) / 2, 32);
    }
}

void changeStatePause(platformer_t* self)
{
    soundPause();
    soundPlaySfx(&(self->soundManager.sndPause), BZR_STEREO);
    self->update = &updatePause;
}

void updatePause(platformer_t* self)
{
    if (((self->gameData.btnState & PB_START) && !(self->gameData.prevBtnState & PB_START)))
    {
        soundResume();
        soundPlaySfx(&(self->soundManager.sndPause), BZR_STEREO);
        self->update = &updateGame;
    }

    mg_drawTileMap(&(self->tilemap));
    mg_drawEntities(&(self->entityManager));
    drawPlatformerHud(&(self->font), &(self->gameData));
    drawPause(&(self->font));
}

void drawPause(font_t* font)
{
    drawText(font, c555, str_pause, (TFT_WIDTH - textWidth(font, str_pause)) / 2, 128);
}

uint16_t getLevelIndex(uint8_t world, uint8_t level)
{
    return (world - 1) * 4 + (level - 1);
}

void changeStateLevelSelect(platformer_t* self)
{
    self->gameData.frameCount   = 0;
    self->menuState             = 0;
    self->menuSelection         = 0;
    self->gameData.level        = 1;
    self->gameData.btnState     = 0;
    self->gameData.prevBtnState = 0;

    mg_loadWsgSet(&(platformer->wsgManager), leveldef[0].defaultWsgSetIndex);
    mg_loadMapFromFile(&(platformer->tilemap), leveldef[0].filename, &platformer->entityManager);
    self->tilemap.mapOffsetX       = 12;
    self->tilemap.mapOffsetY       = 0;
    self->entityManager.viewEntity = NULL;

    if (self->unlockables.levelsCleared & (1 << 12))
    {
        // if hank is defeated, play this other song
        mg_setBgm(&self->soundManager, MG_BGM_LOOKS_LIKE_WE_MADE_IT);
    }
    else
    {
        mg_setBgm(&self->soundManager, MG_BGM_STAGE_SELECT);
    }
    globalMidiPlayerGet(MIDI_BGM)->loop = true;
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
    self->gameData.bgColors = bgGradientMenu;

    self->update = &updateLevelSelect;

    if (self->gameData.trophyEarned >= 0)
    {
        trophyUpdate(&platformerTrophies[self->gameData.trophyEarned], 1, true);
    }
}

void updateLevelSelect(platformer_t* self)
{
    self->gameData.frameCount++;
    if (self->gameData.frameCount > 59)
    {
        self->gameData.frameCount = 0;
    }

    if (self->menuState < 2 && ((self->gameData.btnState & PB_RIGHT) && !(self->gameData.prevBtnState & PB_RIGHT)))
    {
        self->menuState++;
        self->gameData.level = (self->menuState + self->menuSelection * 3) + 1;
        soundPlaySfx(&self->soundManager.sndMenuSelect, MIDI_SFX);
    }
    else if (self->menuState > 0 && ((self->gameData.btnState & PB_LEFT) && !(self->gameData.prevBtnState & PB_LEFT)))
    {
        self->menuState--;
        self->gameData.level = (self->menuState + self->menuSelection * 3) + 1;
        soundPlaySfx(&self->soundManager.sndMenuSelect, MIDI_SFX);
    }

    if (self->menuSelection < 2 && ((self->gameData.btnState & PB_DOWN) && !(self->gameData.prevBtnState & PB_DOWN)))
    {
        self->menuSelection++;
        self->gameData.level = (self->menuState + self->menuSelection * 3) + 1;
        soundPlaySfx(&self->soundManager.sndMenuSelect, MIDI_SFX);
    }
    else if (self->menuSelection > 0 && ((self->gameData.btnState & PB_UP) && !(self->gameData.prevBtnState & PB_UP)))
    {
        self->menuSelection--;
        self->gameData.level = (self->menuState + self->menuSelection * 3) + 1;
        soundPlaySfx(&self->soundManager.sndMenuSelect, MIDI_SFX);
    }

    if ((self->gameData.btnState & PB_A) && !(self->gameData.prevBtnState & PB_A))
    {
        self->gameData.level = (self->menuState + self->menuSelection * 3) + 1;

        uint8_t actualLevel = self->gameData.level;
        if (self->gameData.level == 5)
        {
            if (self->unlockables.levelsCleared & (1 << 5)) // if gauntlet was complete
            {
                actualLevel = 11; // about to start boss rush
                actualLevel += (self->unlockables.levelsCleared & (1 << 11))
                                   ? 1
                                   : 0; // make it final showdown (12) if 11 was complete.
                actualLevel += (self->unlockables.levelsCleared & (1 << 12)) ? 1 : 0; // make it 13 if 12 was complete.
            }
        }
        self->gameData.level = actualLevel;

        bool levelAvailable = !(self->unlockables.levelsCleared & (1 << self->gameData.level));
        if (self->gameData.level == 5)
        {
            levelAvailable = self->unlockables.levelsCleared == 0b11111011110
                             || self->unlockables.levelsCleared == 0b11111111110
                             || self->unlockables.levelsCleared == 0b111111111110
                             || self->unlockables.levelsCleared == 0b1111111111110;
        }
        if (self->gameData.level == 1)
        {
            self->gameData.kineticSkipped = false;
        }
        if (self->gameData.level < 11 && !levelAvailable)
        {
            soundPlaySfx(&(platformer->soundManager.sndMenuDeny), BZR_STEREO);
        }
        else if (self->gameData.level == 13) // new game +
        {
            // Undo all level progress, but keep abilities.
            self->unlockables.levelsCleared = 0;
            savePlatformerUnlockables(self);

            writeNvs32(MG_abilitiesNVSKey, platformer->gameData.abilities);
            mg_initializeGameDataFromTitleScreen(&(platformer->gameData));
            platformer->gameData.level = 10;
            mg_loadWsgSet(&(platformer->wsgManager), leveldef[platformer->gameData.level].defaultWsgSetIndex);
            mg_loadMapFromFile(&(platformer->tilemap), leveldef[platformer->gameData.level].filename,
                               &platformer->entityManager);

            changeStateGame(platformer);
            // every level starts with a cutscene
            midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
            soundPlayBgm(&platformer->soundManager.currentBgm, BZR_STEREO);
            stageStartCutscene(&platformer->gameData);
            changeStateCutscene(platformer);
            return;
        }
        else
        {
            mg_loadWsgSet(&(platformer->wsgManager), leveldef[self->gameData.level].defaultWsgSetIndex);
            mg_loadMapFromFile(&(platformer->tilemap), leveldef[self->gameData.level].filename,
                               &platformer->entityManager);

            changeStateGame(platformer);
            // every level starts with a cutscene
            midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
            soundPlayBgm(&self->soundManager.currentBgm, BZR_STEREO);
            stageStartCutscene(&platformer->gameData);
            changeStateCutscene(platformer);
            return;
        }
    }

    drawLevelSelect(self);
}

void drawLevelSelect(platformer_t* self)
{
    drawText(&self->font, c555, "STAGE SELECT", 90, 8);

    bool levelAvailable = !(self->unlockables.levelsCleared & (1 << self->gameData.level));
    if (self->gameData.level == 5)
    {
        levelAvailable = self->unlockables.levelsCleared == 0b11111011110
                         || self->unlockables.levelsCleared == 0b11111111110
                         || self->unlockables.levelsCleared == 0b111111111110
                         || self->unlockables.levelsCleared == 0b1111111111110;
    }

    drawRectFilled((55 + self->menuState * 64) - self->tilemap.mapOffsetX,
                   (39 + self->menuSelection * 64) - self->tilemap.mapOffsetY,
                   (55 + 66 + self->menuState * 64) - self->tilemap.mapOffsetX,
                   (39 + 66 + self->menuSelection * 64) - self->tilemap.mapOffsetY,
                   levelAvailable ? greenColors[(self->gameData.frameCount >> 3) % 4]
                                  : redColors[(self->gameData.frameCount >> 3) % 4]);

    mg_drawTileMap(&(self->tilemap));

    for (uint8_t j = 0; j < 3; j++)
    {
        for (uint8_t i = 0; i < 3; i++)
        {
            uint8_t idx = (((j * 3) + i) + 1);
            if (self->unlockables.levelsCleared & (1 << idx) && (idx != 5))
            {
                drawWsg(&self->wsgManager.wsgs[MG_WSG_TILE_SOLID_VISIBLE_NONINTERACTIVE_33],
                        (64 + i * 64) - self->tilemap.mapOffsetX, (48 + j * 64) - self->tilemap.mapOffsetY,
                        esp_random() % 2, esp_random() % 2, 0);
            }
            else
            {
                // Special case for Bigma
                if (idx == 5 && (self->unlockables.levelsCleared ^ 0b11111011110)
                    && (self->unlockables.levelsCleared ^ 0b11111111110)
                    && (self->unlockables.levelsCleared ^ 0b111111111110)
                    && (self->unlockables.levelsCleared ^ 0b1111111111110))
                {
                    drawWsg(&self->wsgManager.wsgs[MG_WSG_TILE_SOLID_VISIBLE_NONINTERACTIVE_33],
                            (64 + i * 64) - self->tilemap.mapOffsetX, (48 + j * 64) - self->tilemap.mapOffsetY,
                            esp_random() % 2, esp_random() % 2, 0);
                }
                else
                {
                    // check if the 5th bit is set
                    if ((idx == 5) && (self->unlockables.levelsCleared & (1 << 5))) // if (5 gauntlet) was complete.
                    {
                        uint16_t drawSymbol = MG_WSG_BOSS_RUSH_SYMBOL;
                        drawSymbol += (self->unlockables.levelsCleared & (1 << 11))
                                          ? 1
                                          : 0; // 35 if (11 boss rush) was complete.
                        drawSymbol += (self->unlockables.levelsCleared & (1 << 12))
                                          ? 1
                                          : 0; // 36 if (12 final showdown) was complete
                        drawWsgTile(&self->wsgManager.wsgs[drawSymbol], (64 + i * 64) - self->tilemap.mapOffsetX,
                                    (48 + j * 64) - self->tilemap.mapOffsetY);
                    }
                    else
                    {
                        drawWsgTile(&self->wsgManager.wsgs[MG_WSG_TILE_SOLID_VISIBLE_NONINTERACTIVE_2A + ((j * 3) + i)],
                                    (64 + i * 64) - self->tilemap.mapOffsetX, (48 + j * 64) - self->tilemap.mapOffsetY);
                    }
                }
            }
        }
    }

    if (levelAvailable)
    {
        drawRect(
            (64 + self->menuState * 64) - self->tilemap.mapOffsetX + ((self->gameData.frameCount >> 2) & 0b0111),
            (48 + self->menuSelection * 64) - self->tilemap.mapOffsetY + ((self->gameData.frameCount >> 2) & 0b0111),
            (64 + 48 + self->menuState * 64) - self->tilemap.mapOffsetX - ((self->gameData.frameCount >> 2) & 0b0111),
            (48 + 48 + self->menuSelection * 64) - self->tilemap.mapOffsetY
                - ((self->gameData.frameCount >> 2) & 0b0111),
            highScoreNewEntryColors[self->gameData.frameCount % 4]);
    }
    else
    {
        drawLine((64 + self->menuState * 64) - self->tilemap.mapOffsetX,
                 (48 + self->menuSelection * 64) - self->tilemap.mapOffsetY,
                 (64 + 48 + self->menuState * 64) - self->tilemap.mapOffsetX,
                 (48 + 48 + self->menuSelection * 64) - self->tilemap.mapOffsetY,
                 redColors[self->gameData.frameCount % 4], 0);
        drawLine((64 + self->menuState * 64) - self->tilemap.mapOffsetX,
                 (48 + 48 + self->menuSelection * 64) - self->tilemap.mapOffsetY,
                 (64 + 48 + self->menuState * 64) - self->tilemap.mapOffsetX,
                 (48 + self->menuSelection * 64) - self->tilemap.mapOffsetY, redColors[self->gameData.frameCount % 4],
                 0);
    }
}

// forward declared in mega_pulse_ex_typedef.h
void goToReadyScreen(void)
{
    if (platformer->entityManager.playerEntity != NULL && platformer->entityManager.playerEntity->hp <= 0)
    {
        // if the player and boss reached zero health at the same frame, give the win to the player.
        platformer->entityManager.playerEntity->hp = 1;
    }
    platformer->gameData.canGrabMixtape = true;
    platformer->update                  = &updateReadyScreen;
}

void startCreditMusic(void)
{
    globalMidiPlayerGet(MIDI_BGM)->paused = false;
    globalMidiPlayerGet(MIDI_BGM)->loop   = true;
    mg_setBgm(&platformer->soundManager, MG_BGM_MAXIMUM_HYPE_CREDITS);
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    int16_t songPitches[] = {62, 61, 60, 69, 62, 60, -1, -1};
    setSongPitches(platformer->gameData.cutscene, songPitches);
    soundPlayBgm(&platformer->soundManager.currentBgm, BZR_STEREO);
}

void startPostFightMusic(void)
{
    globalMidiPlayerGet(MIDI_BGM)->paused = false;
    mg_setBgm(&platformer->soundManager, MG_BGM_POST_FIGHT);
    int16_t songPitches[] = {62, 65, 67, 57, -1, -1, -1, -1};
    setSongPitches(platformer->gameData.cutscene, songPitches);
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    soundPlayBgm(&platformer->soundManager.currentBgm, BZR_STEREO);
}

void queueTrophy(void)
{
    if (!platformer->gameData.cheatMode)
    {
        uint8_t level                     = platformer->gameData.level;
        platformer->gameData.trophyEarned = -1; // no trophy by default (handles the gauntlet and rush)
        switch (level)
        {
            case 11: // boss rush is now counted as "saved bigma"
            {
                platformer->gameData.trophyEarned = 8;
                break;
            }
            case 1:
            {
                platformer->gameData.trophyEarned = 0;
                break;
            }
            case 2:
            {
                platformer->gameData.trophyEarned = 1;
                break;
            }
            case 3:
            {
                platformer->gameData.trophyEarned = 2;
                break;
            }
            case 4:
            {
                platformer->gameData.trophyEarned = 3;
                break;
            }
            case 6:
            {
                platformer->gameData.trophyEarned = 4;
                break;
            }
            case 7:
            {
                platformer->gameData.trophyEarned = 5;
                break;
            }
            case 8:
            {
                platformer->gameData.trophyEarned = 6;
                break;
            }
            case 9:
            {
                platformer->gameData.trophyEarned = 7;
                break;
            }
            case 12: // hank
            {
                platformer->gameData.trophyEarned = 9;
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

void startHankMusic(void)
{
    globalMidiPlayerGet(MIDI_BGM)->paused = false;
    mg_setBgm(&platformer->soundManager, MG_BGM_BOSS_HANK_WADDLE);
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    int16_t songPitches[] = {67, 67, 67, 67, 67, 67, 67, 67};
    setSongPitches(platformer->gameData.cutscene, songPitches);
    soundPlayBgm(&platformer->soundManager.currentBgm, BZR_STEREO);
}

void startTrashManMusic(void)
{
    globalMidiPlayerGet(MIDI_BGM)->paused = false;
    mg_setBgm(&platformer->soundManager, MG_BGM_OVO_LIVES);
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    int16_t songPitches[] = {67, 70, 60, 61, 62, 65, 67, 78};
    setSongPitches(platformer->gameData.cutscene, songPitches);
    soundPlayBgm(&platformer->soundManager.currentBgm, BZR_STEREO);
}

void getTrashManTrophy(void)
{
    trophyUpdate(&platformerTrophies[10], 1, true);
}

void startMegajamMusic(void)
{
    globalMidiPlayerGet(MIDI_BGM)->paused = false;
    mg_setBgm(&platformer->soundManager, MG_BGM_THE_FINAL_MEGAJAM);
    midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
    int16_t songPitches[] = {62, 62, 62, 62, 62, 62, 62, 62};
    setSongPitches(platformer->gameData.cutscene, songPitches);
    soundPlayBgm(&platformer->soundManager.currentBgm, BZR_STEREO);
}

void stopMusic(void)
{
    globalMidiPlayerGet(MIDI_BGM)->paused = true;
}

void loseCanOfSalsa(void)
{
    platformer->gameData.abilities &= ~(1U << MG_CAN_OF_SALSA_ABILITY);
}

// forward declared in mega_pulse_ex_typedef.h
void initBossFight(void)
{
    if (platformer->entityManager.bossEntity != NULL)
    {
        platformer->entityManager.bossEntity->state = 0;
    }

    if (platformer->gameData.level == 5)
    {
        platformer->gameData.canGrabMixtape = true;
    }
    else if (platformer->gameData.level == 1 && platformer->gameData.kineticSkipped)
    {
        platformer->gameData.canGrabMixtape = true;
        killEnemy(platformer->entityManager.bossEntity);
    }
    else
    {
        if (platformer->gameData.level == 11)
        {
            // make stage lights blue for kinetic donut at start of the boss rush
            platformer->gameData.bgColors = leveldef[1].bgColors;
        }
        else
        {
            mg_setBgm(&platformer->soundManager, leveldef[platformer->gameData.level].bossBgmIndex);
            midiPlayerResetNewSong(globalMidiPlayerGet(MIDI_BGM));
            soundPlayBgm(&platformer->soundManager.currentBgm, BZR_STEREO);
        }
    }

    platformer->update = &updateReadyScreen;
}

static void megaPulseAddToSwadgePassPacket(swadgePassPacket_t* packet)
{
    addHighScoreToSwadgePassPacket(KEY_SCORES, packet, megaPulseSetSwadgePassHighScore);
}

static int32_t megaPulseGetSwadgePassHighScore(const swadgePassPacket_t* packet)
{
    return packet->megaPulseEx.highScore;
}

static void megaPulseSetSwadgePassHighScore(swadgePassPacket_t* packet, int32_t highScore)
{
    packet->megaPulseEx.highScore = highScore;
}