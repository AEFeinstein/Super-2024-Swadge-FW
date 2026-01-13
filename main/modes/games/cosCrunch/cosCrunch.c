#include "cosCrunch.h"

#include "ccmgBeStrong.h"
#include "ccmgBreakTime.h"
#include "ccmgCatch.h"
#include "ccmgDelivery.h"
#include "ccmgSew.h"
#include "ccmgSlice.h"
#include "ccmgSpray.h"
#include "ccmgThread.h"
#include "cosCrunchUtil.h"
#include "highScores.h"
#include "mainMenu.h"
#include "menuCosCrunchRenderer.h"
#include "nameList.h"
#include "swadge2024.h"
#include "wsgPalette.h"

static const char cosCrunchName[]                    = "Cosplay Crunch";
static const char cosCrunchStartCraftingLbl[]        = "Start Crafting";
static const char* const cosCrunchPlayerOptionLbls[] = {"1P", "2P", "3P", "4P"};
static const int32_t cosCrunchPlayerOptionValues[]   = {1, 2, 3, 4};
static const char cosCrunchHowToPlayLbl[]            = "How To Play";
static const char cosCrunchHighScoresLbl[]           = "High Scores";
static const char cosCrunchExitLbl[]                 = "Exit";

static const char cosCrunchInterludeSpeedUpMsg[] = "Speed up!";
static const char cosCrunchInterludePlayerMsg[]  = "Player %" PRIu8 " get ready!";

static const char cosCrunchGameOverTitle[]   = "Your costumes aren't done!";
static const char cosCrunchYourScoreMsg[]    = "Your score: %" PRIi32;
static const char cosCrunchPlayerScoreMsg[]  = "Player %" PRIu8 ": %" PRIi32;
static const char cosCrunchNewHighScoreMsg[] = "New personal best!";

static const char* cosCrunchHowToPlayText[]
    = {"MAGFest is almost here, but your costumes aren't ready yet! Cut, sew, paint, and craft as fast as you can with "
       "the D-pad and A button. Every time you make a mistake, the Days 'Til MAG counts down.",
       "See how many crafts you can complete before MAGFest arrives. Your score depends on it! You'll never really be "
       "done, but maybe you can get close enough."};

const cosCrunchMicrogame_t* const microgames[] = {
    &ccmgBeStrong, &ccmgBreakTime, &ccmgCatch, &ccmgDelivery, &ccmgSew, &ccmgSlice, &ccmgSpray, &ccmgThread,
};

#define CC_NVS_NAMESPACE      "cc"
#define NVS_KEY_TUTORIAL_SEEN "tutorialSeen"

#define NUM_LIVES                        4
#define MICROGAME_GET_READY_TIME_US      1000000
#define MICROGAME_RESULT_DISPLAY_TIME_US 1800000
#define SPEED_UP_INTERLUDE_TIME_US       1400000
#define PLAYER_INTERLUDE_TIME_US         2500000
#define TIMER_PIXELS_PER_SECOND          10

#define HIGH_SCORE_COUNT 7

#define MICROGAMES_BETWEEN_SPEED_UPS 5
#define SPEED_UP_AMOUNT              .08f

#define MESSAGE_X_OFFSET 25
#define MESSAGE_Y_OFFSET 45
#define TEXT_Y_SPACING   5

#define MESSAGE_BOX_MARGIN    15
#define MESSAGE_BOX_PADDING   10
#define GAME_OVER_SCORE_BOX_Y 165

#define EYES_SLOT_DEFAULT 3
#define EYES_SLOT_HAPPY   4
#define EYES_SLOT_SAD     5
#define EYES_SLOT_DEAD    6
#define EYES_SLOT_SWIRL   7
#define EYES_SWIRL_FRAMES 4

typedef enum
{
    /// Main menu
    CC_MENU,
    /// A microgame is currently loading: don't blank the display or draw anything
    CC_MICROGAME_PENDING,
    /// A microgame is in progress
    CC_MICROGAME_RUNNING,
    /// Displays a message between microgames, e.g., "Speed up!"
    CC_INTERLUDE,
    /// The player has lost, but we still need to unload the last microgame played
    CC_GAME_OVER_PENDING,
    /// Game over screen
    CC_GAME_OVER,
    /// Help screen
    CC_TUTORIAL,
    /// High scores screen
    CC_HIGH_SCORES,
} cosCrunchState;

typedef struct
{
    uint8_t lives;
    int32_t score;
} cosCrunchPlayer_t;

typedef struct
{
    uint8_t tutorialPage;

    uint8_t playerCount;
    uint8_t currentPlayer;
    bool announcePlayer;
    cosCrunchPlayer_t players[4];
    int32_t microgamesAttempted;
    float timeScale;
    bool personalBestAchieved;
    cosCrunchState state;

    struct
    {
        char message[32];
        int64_t timeUs;
        int64_t elapsedUs;
        bool swirlyEyes;
        int8_t swirlFrame;
    } interlude;

    menu_t* menu;
    menuCosCrunchRenderer_t* menuRenderer;

    struct
    {
        const cosCrunchMicrogame_t* game;
        int64_t gameTimeRemainingUs;
        cosCrunchMicrogameState state;
        uint64_t stateElapsedUs;
    } activeMicrogame;

    struct
    {
        const cosCrunchMicrogame_t* game;
        bool successful;
    } previousMicrogame;

    /// Record of microgames completed in the current game. Used for trophy purposes.
    bool successfulMicrogames[ARRAY_SIZE(microgames)];

    /// Used to tint grayscale images (c111, c222, c333, c444). See PALETTE_* defines
    wsgPalette_t tintPalette;

    /// A palette used by microgames to tint grayscale images (c111, c222, c333, c444). See cosCrunchMicrogamePalette()
    wsgPalette_t mgTintPalette;
    int16_t mgTintColorIndex;

    struct
    {
        wsg_t backgroundMat;
        wsg_t backgroundDesk;
        /// Copy of the background that accumulates overdraw
        wsg_t backgroundSplatter;
        wsg_t calendar;
        wsg_t paintLabel;
        wsg_t paintTube;
        wsg_t timerLeft;
        wsg_t timerRight;
        wsg_t menuFold;
    } wsg;
    paletteColor_t backgroundSplatterPixels[TFT_WIDTH * TFT_HEIGHT];

    font_t font;
    font_t bigFont;
    font_t bigFontOutline;

    led_t leds[CONFIG_NUM_LEDS];

    midiFile_t menuBgm;
    midiFile_t gameBgm;
    midiFile_t gameOverBgm;
    midiPlayer_t* bgmPlayer;
    midiPlayer_t* sfxPlayer;
    uint32_t gameBgmOriginalTempo;

    highScores_t highScores;
    swadgesona_t highScoreSonas[HIGH_SCORE_COUNT];
} cosCrunch_t;
cosCrunch_t* cc = NULL;

static void cosCrunchEnterMode(void);
static void cosCrunchExitMode(void);
static bool cosCrunchMenu(const char* label, bool selected, uint32_t value);
static void cosCrunchMainLoop(int64_t elapsedUs);
static void cosCrunchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void cosCrunchResetBackground(void);
static void cosCrunchDisplayMessage(const char* msg);
static void cosCrunchClearLeds(void);
static void cosCrunchDrawTimer(void);
static void cosCrunchAddToSwadgePassPacket(swadgePassPacket_t* packet);
static int32_t cosCrunchGetSwadgePassHighScore(const swadgePassPacket_t* packet);
static void cosCrunchSetSwadgePassHighScore(swadgePassPacket_t* packet, int32_t highScore);

/// This enum's order must match the trophy data order
typedef enum
{
    STORES_CLOSED = 0,
    SCORE_10,
    SCORE_25,
    SCORE_50,
    UNBOTHERED,
    COMPLETE_EVERY_MICROGAME,
    COMPLETE_MULTIPLAYER_GAME,
    BEAT_SP_SCORE,
} ccTrophyIdx;
const trophyData_t ccTrophies[] = {
    {
        .title       = "Store's Closed",
        .description = "Miss out on a clearance bargain",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "10-Foot Rule",
        .description = "Score 10 or more in single player mode",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 10,
    },
    {
        .title       = "Finished Seams",
        .description = "Score 25 or more in single player mode",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 25,
    },
    {
        .title       = "A Costume For Every Day",
        .description = "Score 50 or more in single player mode",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 50,
    },
    {
        .title       = "Unbothered. Moisturized. In My Lane.",
        .description = "Beat \"Break Time\" and \"Be Strong\" back-to-back",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Mixed Media",
        .description = "Complete every microgame in one session",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
    },
    {
        .title       = "Group Cosplay",
        .description = "Play a multiplayer game",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "Hallway Photo",
        .description = "Beat a SwadgePass score",
        .image       = NO_IMAGE_SET,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
};
const trophySettings_t ccTrophySettings = {
    .drawFromBottom   = true,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = "ccTrophies",
};
const trophyDataList_t ccTrophyData = {
    .settings = &ccTrophySettings,
    .list     = ccTrophies,
    .length   = ARRAY_SIZE(ccTrophies),
};

swadgeMode_t cosCrunchMode = {
    .modeName                 = cosCrunchName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = cosCrunchEnterMode,
    .fnExitMode               = cosCrunchExitMode,
    .fnMainLoop               = cosCrunchMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = cosCrunchBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
    .fnAddToSwadgePassPacket  = cosCrunchAddToSwadgePassPacket,
    .trophyData               = &ccTrophyData,
};

/// Uncomment this to play the specified game repeatedly instead of randomizing.
/// Also enables the FPS counter.
// #define DEV_MODE_MICROGAME &ccmgWhatever

tintColor_t const blueTimerTintColor   = {c013, c125, c235, 0};
tintColor_t const yellowTimerTintColor = {c430, c540, c554, 0};
tintColor_t const redTimerTintColor    = {c300, c500, c533, 0};
led_t const blueLedColor               = {.r = 0, .g = 0, .b = 255};
led_t const yellowLedColor             = {.r = 255, .g = 255, .b = 0};
led_t const redLedColor                = {.r = 255, .g = 0, .b = 0};

/// Colors randomly given to games that request a color
tintColor_t tintColors[] = {
    {c010, c232, c343, 0}, // Green
    {c100, c210, c310, 0}, // Brown
    {c134, c245, c355, 0}, // Sky blue
    {c012, c023, c134, 0}, // Aqua
    {c102, c213, c324, 0}, // Violet
    {c304, c415, c525, 0}, // Magenta
    {c440, c551, c554, 0}, // Yellow
    {c420, c530, c541, 0}, // Orange
    {c412, c523, c534, 0}, // Pink
};

static void cosCrunchEnterMode(void)
{
    setFrameRateUs(1000000 / 60);

    cc = heap_caps_calloc(1, sizeof(cosCrunch_t), MALLOC_CAP_8BIT);

    int32_t tutorialSeen;
    if (readNamespaceNvs32(CC_NVS_NAMESPACE, NVS_KEY_TUTORIAL_SEEN, &tutorialSeen))
    {
        cc->state = CC_MENU;
    }
    else
    {
        cc->state        = CC_TUTORIAL;
        cc->tutorialPage = 0;
        writeNamespaceNvs32(CC_NVS_NAMESPACE, NVS_KEY_TUTORIAL_SEEN, true);
    }

    cc->menu                    = initMenu(cosCrunchName, cosCrunchMenu);
    settingParam_t playerBounds = {
        .min = cosCrunchPlayerOptionValues[0],
        .max = cosCrunchPlayerOptionValues[ARRAY_SIZE(cosCrunchPlayerOptionValues) - 1],
    };
    addSettingsOptionsItemToMenu(cc->menu, cosCrunchStartCraftingLbl, cosCrunchPlayerOptionLbls,
                                 cosCrunchPlayerOptionValues, ARRAY_SIZE(cosCrunchPlayerOptionLbls), &playerBounds, 1);
    addSingleItemToMenu(cc->menu, cosCrunchHighScoresLbl);
    addSingleItemToMenu(cc->menu, cosCrunchHowToPlayLbl);
    addSingleItemToMenu(cc->menu, cosCrunchExitLbl);
    cc->menuRenderer = initMenuCosCrunchRenderer(&cc->bigFont, &cc->bigFontOutline, &cc->font);

    wsgPaletteReset(&cc->tintPalette);
    wsgPaletteReset(&cc->mgTintPalette);
    cc->mgTintColorIndex = -1; // Will be initialized when a microgame requests a tint color

    loadWsg(CC_BACKGROUND_MAT_WSG, &cc->wsg.backgroundMat, true);
    loadWsg(CC_BACKGROUND_DESK_WSG, &cc->wsg.backgroundDesk, false);
    loadWsg(CC_CALENDAR_WSG, &cc->wsg.calendar, false);
    loadWsg(CC_PAINT_LABEL_WSG, &cc->wsg.paintLabel, false);
    loadWsg(CC_PAINT_TUBE_WSG, &cc->wsg.paintTube, false);
    loadWsg(CC_TIMER_LEFT_WSG, &cc->wsg.timerLeft, false);
    loadWsg(CC_TIMER_RIGHT_WSG, &cc->wsg.timerRight, false);
    loadWsg(CC_MENU_FOLD_WSG, &cc->wsg.menuFold, false);

    cc->wsg.backgroundSplatter.w  = TFT_WIDTH;
    cc->wsg.backgroundSplatter.h  = TFT_HEIGHT;
    cc->wsg.backgroundSplatter.px = cc->backgroundSplatterPixels;
    cosCrunchResetBackground();

    cc->font = *getSysFont();
    loadFont(RIGHTEOUS_150_FONT, &cc->bigFont, false);
    makeOutlineFont(&cc->bigFont, &cc->bigFontOutline, false);

    loadMidiFile(COSPLAY_CRUNCH_MENU_MID, &cc->menuBgm, true);
    loadMidiFile(COSPLAY_CRUNCH_BGM_MID, &cc->gameBgm, true);
    loadMidiFile(COSPLAY_CRUNCH_GAME_OVER_MID, &cc->gameOverBgm, true);

    cc->bgmPlayer       = globalMidiPlayerGet(MIDI_BGM);
    cc->bgmPlayer->loop = true;
    midiGmOn(cc->bgmPlayer);
    globalMidiPlayerPlaySong(&cc->menuBgm, MIDI_BGM);

    cc->sfxPlayer = globalMidiPlayerGet(MIDI_SFX);

    cosCrunchClearLeds();

    cc->highScores.highScoreCount = HIGH_SCORE_COUNT;
    initHighScores(&cc->highScores, CC_NVS_NAMESPACE);

    list_t swadgePasses = {0};
    // It's okay to get already-used passes, since the high score table only saves one per SP user.
    getSwadgePasses(&swadgePasses, &cosCrunchMode, true);
    saveHighScoresFromSwadgePass(&cc->highScores, CC_NVS_NAMESPACE, swadgePasses, cosCrunchGetSwadgePassHighScore);
    freeSwadgePasses(&swadgePasses);
}

static void cosCrunchExitMode(void)
{
    if (cc->activeMicrogame.game != NULL)
    {
        cc->activeMicrogame.game->fnDestroyMicrogame(cc->activeMicrogame.state == CC_MG_CELEBRATING);
    }

    deinitMenuCosCrunchRenderer(cc->menuRenderer);
    deinitMenu(cc->menu);

    freeWsg(&cc->wsg.backgroundMat);
    freeWsg(&cc->wsg.backgroundDesk);
    freeWsg(&cc->wsg.calendar);
    freeWsg(&cc->wsg.paintLabel);
    freeWsg(&cc->wsg.paintTube);
    freeWsg(&cc->wsg.timerLeft);
    freeWsg(&cc->wsg.timerRight);
    freeWsg(&cc->wsg.menuFold);

    freeFont(&cc->bigFont);
    freeFont(&cc->bigFontOutline);

    globalMidiPlayerStop(MIDI_BGM);
    globalMidiPlayerStop(MIDI_SFX);
    unloadMidiFile(&cc->menuBgm);
    unloadMidiFile(&cc->gameBgm);
    unloadMidiFile(&cc->gameOverBgm);

    freeHighScoreSonas(&cc->highScores, cc->highScoreSonas);

    heap_caps_free(cc);
}

static bool cosCrunchMenu(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (label == cosCrunchStartCraftingLbl)
        {
            for (uint8_t i = 0; i < ARRAY_SIZE(cc->players); i++)
            {
                cc->players[i].lives = NUM_LIVES;
                cc->players[i].score = 0;
            }
            cc->playerCount         = value;
            cc->announcePlayer      = cc->playerCount > 1;
            cc->currentPlayer       = 0;
            cc->microgamesAttempted = 0;
            cc->timeScale           = 1.0f;
            cc->state               = CC_MICROGAME_PENDING;
            // Reset the background splatter for the next game
            cosCrunchResetBackground();

            // Reset per-game trophy tracking
            cc->previousMicrogame.game       = NULL;
            cc->previousMicrogame.successful = false;
            for (int i = 0; i < ARRAY_SIZE(microgames); i++)
            {
                cc->successfulMicrogames[i] = false;
            }

            globalMidiPlayerPlaySong(&cc->gameBgm, MIDI_BGM);

            // Loading images will disable the default blink animation
            ch32v003WriteBitmapAsset(EYES_SLOT_DEFAULT, EYES_DEFAULT_GS);
            ch32v003WriteBitmapAsset(EYES_SLOT_HAPPY, EYES_HAPPY_GS);
            ch32v003WriteBitmapAsset(EYES_SLOT_SAD, EYES_SAD_GS);
            ch32v003WriteBitmapAsset(EYES_SLOT_DEAD, EYES_DEAD_GS);
            ch32v003WriteBitmapAsset(EYES_SLOT_SWIRL + 0, EYES_SWIRL_0_GS);
            ch32v003WriteBitmapAsset(EYES_SLOT_SWIRL + 1, EYES_SWIRL_1_GS);
            ch32v003WriteBitmapAsset(EYES_SLOT_SWIRL + 2, EYES_SWIRL_2_GS);
            ch32v003WriteBitmapAsset(EYES_SLOT_SWIRL + 3, EYES_SWIRL_3_GS);
            ch32v003SelectBitmap(EYES_SLOT_DEFAULT);
        }
        else if (label == cosCrunchHighScoresLbl)
        {
            initHighScoreSonas(&cc->highScores, cc->highScoreSonas);
            cc->state = CC_HIGH_SCORES;
        }
        else if (label == cosCrunchHowToPlayLbl)
        {
            cc->state        = CC_TUTORIAL;
            cc->tutorialPage = 0;
        }
        else if (label == cosCrunchExitLbl)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    return false;
}

static void cosCrunchMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evts[42] = {0}; // Array length stolen from initButtons() in hdw-btn.c
    uint8_t evtCount     = 0;
    buttonEvt_t evt      = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        evts[evtCount++] = evt;
        if (cc->state == CC_MENU)
        {
            cc->menu = menuButton(cc->menu, evt);
        }
        else if (cc->state == CC_TUTORIAL)
        {
            if (evt.button == PB_A && evt.down)
            {
                cc->tutorialPage++;
                if (cc->tutorialPage >= ARRAY_SIZE(cosCrunchHowToPlayText))
                {
                    cc->state = CC_MENU;
                }
            }
            else if (evt.button == PB_B && evt.down)
            {
                cc->tutorialPage = MAX(cc->tutorialPage - 1, 0);
            }
        }
        else if (cc->state == CC_GAME_OVER && (evt.button == PB_A || evt.button == PB_B) && evt.down)
        {
            // Loading the cfun makes the midi player stutter, so don't let them continue during the game over song
            if (cc->bgmPlayer->paused)
            {
                // Resume default blink animations while in menu/high scores/tutorial
                ch32v003RunBinaryAsset(MATRIX_BLINKS_CFUN_BIN);

                cc->bgmPlayer->loop = true;
                globalMidiPlayerPlaySong(&cc->menuBgm, MIDI_BGM);

                cc->state = CC_MENU;
            }
        }
        else if (cc->state == CC_HIGH_SCORES && (evt.button == PB_A || evt.button == PB_B) && evt.down)
        {
            cc->state = CC_MENU;
        }
    }

    if (cc->state == CC_MICROGAME_PENDING && cc->announcePlayer)
    {
        cc->announcePlayer = false;
        cc->state          = CC_INTERLUDE;
        snprintf(cc->interlude.message, sizeof(cc->interlude.message), cosCrunchInterludePlayerMsg,
                 cc->currentPlayer + 1);
        cc->interlude.timeUs     = PLAYER_INTERLUDE_TIME_US;
        cc->interlude.elapsedUs  = 0;
        cc->interlude.swirlyEyes = false;
        ch32v003SelectBitmap(EYES_SLOT_DEFAULT);
    }

    switch (cc->state)
    {
        case CC_MENU:
            drawMenuCosCrunch(cc->menu, cc->menuRenderer, elapsedUs);
            break;

        case CC_INTERLUDE:
        {
            if (cc->activeMicrogame.game != NULL)
            {
                midiAllSoundOff(cc->sfxPlayer);
                cc->activeMicrogame.game->fnDestroyMicrogame(cc->activeMicrogame.state == CC_MG_CELEBRATING);
                cc->activeMicrogame.game = NULL;
            }

            cosCrunchDisplayMessage(cc->interlude.message);
            cc->interlude.elapsedUs += elapsedUs * cc->timeScale;
            if (cc->interlude.elapsedUs >= cc->interlude.timeUs)
            {
                cc->state = CC_MICROGAME_PENDING;
            }
            else if (cc->interlude.swirlyEyes)
            {
                int64_t frame = cc->interlude.elapsedUs / (cc->interlude.timeUs / EYES_SWIRL_FRAMES);
                if (cc->interlude.swirlFrame != frame)
                {
                    ch32v003SelectBitmap(EYES_SLOT_SWIRL + frame);
                    cc->interlude.swirlFrame = frame;
                }
            }

            cosCrunchClearLeds();

            break;
        }

        case CC_MICROGAME_PENDING:
            if (cc->activeMicrogame.game != NULL)
            {
                cc->activeMicrogame.game->fnDestroyMicrogame(cc->activeMicrogame.state == CC_MG_CELEBRATING);
            }

            // Reset anything that the previous microgame might have monkeyed with
            midiPlayerReset(cc->sfxPlayer);
            midiGmOn(cc->sfxPlayer);
            cc->sfxPlayer->mode = MIDI_STREAMING;
            midiPause(cc->sfxPlayer, false);

#ifdef DEV_MODE_MICROGAME
            cc->activeMicrogame.game = DEV_MODE_MICROGAME;
#else
            const cosCrunchMicrogame_t* newMicrogame;
            do
            {
                newMicrogame = microgames[esp_random() % ARRAY_SIZE(microgames)];
            } while (cc->previousMicrogame.game != NULL && cc->previousMicrogame.game == newMicrogame);
            cc->activeMicrogame.game = newMicrogame;
#endif
            cc->activeMicrogame.game->fnInitMicrogame();

            cc->activeMicrogame.gameTimeRemainingUs = cc->activeMicrogame.game->timeoutUs;
            cc->activeMicrogame.state               = CC_MG_GET_READY;
            cc->activeMicrogame.stateElapsedUs      = 0;
            cc->state                               = CC_MICROGAME_RUNNING;

            ch32v003SelectBitmap(EYES_SLOT_DEFAULT);
            break;

        case CC_MICROGAME_RUNNING:
        {
            cc->activeMicrogame.stateElapsedUs += elapsedUs * cc->timeScale;
            cc->activeMicrogame.game->fnMainLoop(elapsedUs * cc->timeScale, cc->activeMicrogame.gameTimeRemainingUs,
                                                 cc->timeScale, cc->activeMicrogame.state, evts, evtCount);

            switch (cc->activeMicrogame.state)
            {
                case CC_MG_GET_READY:
                    if (cc->activeMicrogame.stateElapsedUs >= MICROGAME_GET_READY_TIME_US)
                    {
                        cc->activeMicrogame.state          = CC_MG_PLAYING;
                        cc->activeMicrogame.stateElapsedUs = 0;
                    }
                    break;

                case CC_MG_PLAYING:
                    cc->activeMicrogame.gameTimeRemainingUs
                        = MAX(cc->activeMicrogame.gameTimeRemainingUs - elapsedUs * cc->timeScale, 0);
                    if (cc->activeMicrogame.gameTimeRemainingUs == 0)
                    {
                        if (cc->activeMicrogame.game->fnMicrogameTimeout != NULL)
                        {
                            cosCrunchMicrogameResult(cc->activeMicrogame.game->fnMicrogameTimeout());
                        }
                        else
                        {
                            cosCrunchMicrogameResult(false);
                        }
                    }
                    break;

                case CC_MG_CELEBRATING:
                case CC_MG_DESPAIRING:
                {
                    const char* msg = NULL;
                    if (cc->activeMicrogame.state == CC_MG_CELEBRATING)
                    {
                        msg = cc->activeMicrogame.game->successMsg;
                    }
                    else if (cc->activeMicrogame.state == CC_MG_DESPAIRING)
                    {
                        msg = cc->activeMicrogame.game->failureMsg;
                    }
                    if (msg != NULL)
                    {
                        cosCrunchDisplayMessage(msg);
                    }

                    uint64_t resultDisplayTimeUs = cc->activeMicrogame.game->resultDisplayTimeUs;
                    if (resultDisplayTimeUs == 0)
                    {
                        resultDisplayTimeUs = MICROGAME_RESULT_DISPLAY_TIME_US;
                    }
                    if (cc->activeMicrogame.stateElapsedUs >= resultDisplayTimeUs)
                    {
                        uint8_t totalLives = 0;
                        for (uint8_t i = 0; i < cc->playerCount; i++)
                        {
                            totalLives += cc->players[i].lives;
                        }
                        if (totalLives == 0)
                        {
                            cc->state = CC_GAME_OVER_PENDING;
                            ch32v003SelectBitmap(EYES_SLOT_DEAD);
                        }
                        else
                        {
                            if (cc->activeMicrogame.state == CC_MG_DESPAIRING && cc->playerCount > 1)
                            {
                                cc->currentPlayer  = (cc->currentPlayer + 1) % cc->playerCount;
                                cc->announcePlayer = true;
                            }

                            if (cc->microgamesAttempted > 0
                                && cc->microgamesAttempted % MICROGAMES_BETWEEN_SPEED_UPS == 0)
                            {
                                cc->timeScale += SPEED_UP_AMOUNT;
                                cc->state = CC_INTERLUDE;
                                strcpy(cc->interlude.message, cosCrunchInterludeSpeedUpMsg);
                                cc->interlude.timeUs     = SPEED_UP_INTERLUDE_TIME_US;
                                cc->interlude.elapsedUs  = 0;
                                cc->interlude.swirlyEyes = true;
                                cc->interlude.swirlFrame = -1;

                                if (cc->gameBgmOriginalTempo == 0)
                                {
                                    cc->gameBgmOriginalTempo = cc->bgmPlayer->tempo;
                                }
                                midiSetTempo(cc->bgmPlayer, cc->gameBgmOriginalTempo / cc->timeScale);
                            }
                            else
                            {
                                cc->state = CC_MICROGAME_PENDING;
                            }
                        }
                    }
                    break;
                }
            }

            if (cc->activeMicrogame.state == CC_MG_GET_READY)
            {
                cosCrunchDisplayMessage(cc->activeMicrogame.game->verb);
            }

            if (cc->activeMicrogame.game->fnBackgroundDrawCallback != NULL)
            {
                drawWsgTile(&cc->wsg.backgroundDesk, 0, TFT_HEIGHT - cc->wsg.backgroundDesk.h);
            }

            cosCrunchDrawTimer();

            drawWsgSimple(&cc->wsg.calendar, TFT_WIDTH - cc->wsg.calendar.w - 7, TFT_HEIGHT - cc->wsg.calendar.h - 1);
            char buf[4];
            snprintf(buf, sizeof(buf), "%d", cc->players[cc->currentPlayer].lives);
            uint16_t tw = textWidth(&cc->font, buf);
            drawText(&cc->font, c000, buf, TFT_WIDTH - 25 - tw, TFT_HEIGHT - cc->font.height - 19);

            break;
        }

        case CC_GAME_OVER_PENDING:
        {
            cc->state = CC_GAME_OVER;
            cc->activeMicrogame.game->fnDestroyMicrogame(cc->activeMicrogame.state == CC_MG_CELEBRATING);
            cc->activeMicrogame.game = NULL;

            if (cc->playerCount == 1)
            {
                int32_t lowestSwadgePassScore = INT32_MAX;
                for (int i = 0; i < HIGH_SCORE_COUNT; i++)
                {
                    int32_t score = cc->highScores.highScores[i].score;
                    if (cc->highScores.highScores[i].swadgesona.packedName != 0 && score > 0)
                    {
                        lowestSwadgePassScore = MIN(lowestSwadgePassScore, score);
                    }
                }

                cc->personalBestAchieved = cc->players[0].score > cc->highScores.userHighScore;
                score_t scores[]         = {{.score = cc->players[0].score, .spKey = {0}, .swadgesona = {0}}};
                updateHighScores(&cc->highScores, CC_NVS_NAMESPACE, scores, ARRAY_SIZE(scores));

                trophyUpdateMilestone(&ccTrophies[SCORE_10], cc->players[0].score, 50);
                trophyUpdateMilestone(&ccTrophies[SCORE_25], cc->players[0].score, 20);
                trophyUpdateMilestone(&ccTrophies[SCORE_50], cc->players[0].score, 20);

                if (cc->players[0].score > lowestSwadgePassScore)
                {
                    trophyUpdate(&ccTrophies[BEAT_SP_SCORE], 1, true);
                }
            }
            else
            {
                cc->personalBestAchieved = false;
                trophyUpdate(&ccTrophies[COMPLETE_MULTIPLAYER_GAME], 1, true);
            }

            cosCrunchClearLeds();

            cc->bgmPlayer->loop = false;
            globalMidiPlayerPlaySong(&cc->gameOverBgm, MIDI_BGM);
            break;
        }

        case CC_GAME_OVER:
        {
            cosCrunchDisplayMessage(cosCrunchGameOverTitle);

            drawMessageBox(MESSAGE_BOX_MARGIN, GAME_OVER_SCORE_BOX_Y, TFT_WIDTH - MESSAGE_BOX_MARGIN,
                           GAME_OVER_SCORE_BOX_Y + MESSAGE_BOX_PADDING * 2 + cc->font.height * 2 + TEXT_Y_SPACING,
                           cc->wsg.menuFold);

            char buf[32];
            if (cc->playerCount == 1)
            {
                int16_t yOff = GAME_OVER_SCORE_BOX_Y + MESSAGE_BOX_PADDING;
                snprintf(buf, sizeof(buf), cosCrunchYourScoreMsg, cc->players[0].score);
                uint16_t tw = textWidth(&cc->font, buf);
                drawText(&cc->font, c000, buf, (TFT_WIDTH - tw) / 2, yOff);

                if (cc->personalBestAchieved)
                {
                    yOff += cc->font.height + TEXT_Y_SPACING;
                    tw = textWidth(&cc->font, cosCrunchNewHighScoreMsg);
                    drawText(&cc->font, c000, cosCrunchNewHighScoreMsg, (TFT_WIDTH - tw) / 2, yOff);
                }
            }
            else
            {
                for (uint8_t i = 0; i < cc->playerCount; i++)
                {
                    snprintf(buf, sizeof(buf), cosCrunchPlayerScoreMsg, i + 1, cc->players[i].score);
                    int16_t xOff;
                    if (i % 2 == 0)
                    {
                        xOff = MESSAGE_X_OFFSET;
                    }
                    else
                    {
                        xOff = TFT_WIDTH / 2;
                    }
                    int16_t yOff = GAME_OVER_SCORE_BOX_Y + MESSAGE_BOX_PADDING;
                    if (i >= 2)
                    {
                        yOff += cc->font.height + TEXT_Y_SPACING;
                    }
                    drawText(&cc->font, c000, buf, xOff, yOff);
                    yOff += cc->font.height + TEXT_Y_SPACING;
                }
            }

            break;
        }

        case CC_TUTORIAL:
        {
            uint16_t tw = textWidth(&cc->bigFont, cosCrunchHowToPlayLbl);
            drawText(&cc->bigFont, c555, cosCrunchHowToPlayLbl, (TFT_WIDTH - tw) / 2, 15);
            drawText(&cc->bigFontOutline, c000, cosCrunchHowToPlayLbl, (TFT_WIDTH - tw) / 2, 15);

            const char* msgText = cosCrunchHowToPlayText[cc->tutorialPage];
            int16_t xOff = 30, yOff = 85;
            int16_t textHeight = textWordWrapHeight(&cc->font, msgText, TFT_WIDTH - xOff * 2, TFT_HEIGHT - yOff - xOff);
            drawMessageBox(20, 75, TFT_WIDTH - 20, 75 + textHeight + 20, cc->wsg.menuFold);
            drawTextWordWrap(&cc->font, c000, msgText, &xOff, &yOff, TFT_WIDTH - xOff, TFT_HEIGHT - xOff);
            break;
        }

        case CC_HIGH_SCORES:
        {
            uint16_t tw = textWidth(&cc->bigFont, cosCrunchHighScoresLbl);
            drawText(&cc->bigFont, c555, cosCrunchHighScoresLbl, (TFT_WIDTH - tw) / 2, -2);
            drawText(&cc->bigFontOutline, c000, cosCrunchHighScoresLbl, (TFT_WIDTH - tw) / 2, -2);

            drawMessageBox(11, 53, TFT_WIDTH - 11, TFT_HEIGHT, cc->wsg.menuFold);

            int16_t yOff        = 64;
            uint16_t scoreWidth = textWidth(&cc->font, "0000");
            for (int i = 0; i < HIGH_SCORE_COUNT; i++)
            {
                if (cc->highScores.highScores[i].score > 0)
                {
                    drawWsgSimpleHalf(&cc->highScoreSonas[i].image, 12, yOff - 10);
                    drawTextEllipsize(&cc->font, c000, cc->highScoreSonas[i].name.nameBuffer, 47, yOff,
                                      TFT_WIDTH - 67 - scoreWidth - 5, false);

                    char buf[16];
                    snprintf(buf, sizeof(buf), "%" PRIi32, cc->highScores.highScores[i].score);
                    tw = textWidth(&cc->font, buf);
                    drawText(&cc->font, c000, buf, TFT_WIDTH - tw - 18, yOff);
                    yOff += cc->font.height + TEXT_Y_SPACING + 11;
                }
            }

            break;
        }
    }

    // Tempo resets when the BGM loops, so re-reset it
    if ((cc->state == CC_MICROGAME_PENDING || cc->state == CC_MICROGAME_RUNNING || cc->state == CC_INTERLUDE)
        && cc->gameBgmOriginalTempo != 0)
    {
        uint32_t scaledTempo = cc->gameBgmOriginalTempo / cc->timeScale;
        if (cc->bgmPlayer->tempo != scaledTempo)
        {
            midiSetTempo(cc->bgmPlayer, scaledTempo);
        }
    }

#ifdef DEV_MODE_MICROGAME
    DRAW_FPS_COUNTER(cc->font);
#endif
}

static void cosCrunchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    switch (cc->state)
    {
        case CC_MENU:
        case CC_INTERLUDE:
        case CC_MICROGAME_RUNNING:
        case CC_GAME_OVER:
        case CC_TUTORIAL:
        case CC_HIGH_SCORES:
        {
            if (cc->state != CC_INTERLUDE && cc->activeMicrogame.game != NULL
                && cc->activeMicrogame.game->fnBackgroundDrawCallback != NULL)
            {
                if (y <= TFT_HEIGHT - cc->wsg.backgroundDesk.h)
                {
                    cc->activeMicrogame.game->fnBackgroundDrawCallback(
                        x, y, w, MIN(h, TFT_HEIGHT - y - cc->wsg.backgroundDesk.h), up, upNum);
                }
            }
            else
            {
                paletteColor_t* tftFb = getPxTftFramebuffer();
                memcpy(&tftFb[y * TFT_WIDTH + x], &cc->wsg.backgroundSplatter.px[y * TFT_WIDTH + x],
                       w * h * sizeof(paletteColor_t));
            }
            break;
        }

        case CC_MICROGAME_PENDING: // Nothing is drawn while loading a microgame, so do nothing to prevent flicker
        case CC_GAME_OVER_PENDING: // Same goes for unloading
            break;
    }
}

static void cosCrunchResetBackground()
{
    drawToCanvas(cc->wsg.backgroundSplatter, cc->wsg.backgroundDesk, 0, TFT_HEIGHT - cc->wsg.backgroundDesk.h);
    drawToCanvas(cc->wsg.backgroundSplatter, cc->wsg.backgroundMat, 0, 0);
}

static void cosCrunchDisplayMessage(const char* msg)
{
    int16_t xOffset = MESSAGE_X_OFFSET;
    int16_t yOffset = MESSAGE_Y_OFFSET;
    drawTextWordWrapCentered(&cc->bigFont, c555, msg, &xOffset, &yOffset, TFT_WIDTH - MESSAGE_X_OFFSET,
                             TFT_HEIGHT - MESSAGE_Y_OFFSET);
    xOffset = MESSAGE_X_OFFSET;
    yOffset = MESSAGE_Y_OFFSET;
    drawTextWordWrapCentered(&cc->bigFontOutline, c000, msg, &xOffset, &yOffset, TFT_WIDTH - MESSAGE_X_OFFSET,
                             TFT_HEIGHT - MESSAGE_Y_OFFSET);
}

static void cosCrunchClearLeds(void)
{
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        cc->leds[i].r = 0;
        cc->leds[i].g = 0;
        cc->leds[i].b = 0;
    }
    setLeds(cc->leds, CONFIG_NUM_LEDS);
}

static void cosCrunchDrawTimer()
{
    const tintColor_t* timerTintColor;
    led_t const* ledColor;
    bool topLeds, middleLeds, bottomLeds;
    if (cc->activeMicrogame.gameTimeRemainingUs <= cc->activeMicrogame.game->timeoutUs / 4)
    {
        timerTintColor = &redTimerTintColor;

        ledColor   = &redLedColor;
        topLeds    = false;
        middleLeds = false;
        bottomLeds = cc->activeMicrogame.gameTimeRemainingUs > 0;
    }
    else if (cc->activeMicrogame.gameTimeRemainingUs <= cc->activeMicrogame.game->timeoutUs / 2)
    {
        timerTintColor = &yellowTimerTintColor;

        ledColor   = &yellowLedColor;
        topLeds    = false;
        middleLeds = true;
        bottomLeds = true;
    }
    else
    {
        timerTintColor = &blueTimerTintColor;

        ledColor   = &blueLedColor;
        topLeds    = true;
        middleLeds = true;
        bottomLeds = true;
    }

    for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
    {
        // LED indices as laid out on the swadge:
        // 5 0
        // 3 2
        // 4 1
        if ((bottomLeds && (i == 1 || i == 4)) || (middleLeds && (i == 2 || i == 3)) || (topLeds && (i == 0 || i == 5)))
        {
            cc->leds[i].r = ledColor->r;
            cc->leds[i].g = ledColor->g;
            cc->leds[i].b = ledColor->b;
        }
        else
        {
            cc->leds[i].r = 0;
            cc->leds[i].g = 0;
            cc->leds[i].b = 0;
        }
    }
    setLeds(cc->leds, CONFIG_NUM_LEDS);

    tintPalette(&cc->tintPalette, timerTintColor);

    if (cc->activeMicrogame.gameTimeRemainingUs > 0)
    {
        uint8_t timer_width = TIMER_PIXELS_PER_SECOND * cc->activeMicrogame.gameTimeRemainingUs / 1000000.f;

        // These offsets are to animate the paint going back into the tube when the timer nears the end
        int8_t offsetX = MIN(timer_width - cc->wsg.timerLeft.w, 0);
        int8_t offsetY = 0;
        if (timer_width == 2 || timer_width == 1)
        {
            offsetY = -1;
        }
        else if (timer_width == 0)
        {
            offsetY = -2;
        }

        // Squiggly bit near the tube
        drawWsgPaletteSimple(&cc->wsg.timerLeft, 61 + offsetX, TFT_HEIGHT - cc->wsg.timerLeft.h - 11 + offsetY,
                             &cc->tintPalette);

        // Long straight paint line
        int16_t timerX = 61 + offsetX + cc->wsg.timerLeft.w;
        int16_t timerY = TFT_HEIGHT - 17 + offsetY;
        fillDisplayArea(timerX, timerY, timerX + timer_width, timerY + 4, timerTintColor->base);

        // Paint line highlight, lowlight, shadow
        drawLineFast(timerX, timerY + 1, timerX + timer_width, timerY + 1, timerTintColor->highlight);
        drawLineFast(timerX, timerY + 4, timerX + timer_width, timerY + 4, timerTintColor->lowlight);
        drawLineFast(timerX, timerY + 5, timerX + timer_width, timerY + 5, c210);

        // Round end cap
        drawWsgPaletteSimple(&cc->wsg.timerRight, 66 + timer_width + offsetX,
                             TFT_HEIGHT - cc->wsg.timerRight.h - 11 + offsetY, &cc->tintPalette);
    }

    drawWsgSimple(&cc->wsg.paintTube, 8, TFT_HEIGHT - cc->wsg.paintTube.h - 4);
    drawWsgPaletteSimple(&cc->wsg.paintLabel, 37, TFT_HEIGHT - cc->wsg.paintLabel.h - 8, &cc->tintPalette);
}

static void cosCrunchAddToSwadgePassPacket(swadgePassPacket_t* packet)
{
    addHighScoreToSwadgePassPacket(CC_NVS_NAMESPACE, packet, cosCrunchSetSwadgePassHighScore);
}

static int32_t cosCrunchGetSwadgePassHighScore(const swadgePassPacket_t* packet)
{
    return packet->cosCrunch.highScore;
}

static void cosCrunchSetSwadgePassHighScore(swadgePassPacket_t* packet, int32_t highScore)
{
    packet->cosCrunch.highScore = highScore;
}

const tintColor_t* cosCrunchMicrogameGetTintColor()
{
    // Pick a random color that's different from the last one we used
    int16_t tintColorIndex;
    do
    {
        tintColorIndex = esp_random() % ARRAY_SIZE(tintColors);
    } while (tintColorIndex == cc->mgTintColorIndex);
    cc->mgTintColorIndex = tintColorIndex;

    return &tintColors[cc->mgTintColorIndex];
}

wsgPalette_t* cosCrunchMicrogameGetWsgPalette(const tintColor_t* tintColor)
{
    wsgPaletteReset(&cc->mgTintPalette);
    tintPalette(&cc->mgTintPalette, tintColor);
    return &cc->mgTintPalette;
}

void cosCrunchMicrogamePersistSplatter(wsg_t wsg, uint16_t x, uint16_t y)
{
    drawToCanvas(cc->wsg.backgroundSplatter, wsg, x, y);
}

void cosCrunchMicrogameResult(bool successful)
{
    if (cc->state == CC_MICROGAME_RUNNING && cc->activeMicrogame.state == CC_MG_PLAYING)
    {
        cc->microgamesAttempted++;
        if (successful)
        {
            cc->players[cc->currentPlayer].score++;
            cc->activeMicrogame.state = CC_MG_CELEBRATING;
            ch32v003SelectBitmap(EYES_SLOT_HAPPY);
        }
        else
        {
            cc->players[cc->currentPlayer].lives--;
            cc->activeMicrogame.state = CC_MG_DESPAIRING;
            ch32v003SelectBitmap(EYES_SLOT_SAD);
        }
        cc->activeMicrogame.stateElapsedUs = 0;

        if (!successful && cc->activeMicrogame.game == &ccmgCatch)
        {
            trophyUpdate(&ccTrophies[STORES_CLOSED], 1, true);
        }

        if (successful && cc->previousMicrogame.successful
            && ((cc->activeMicrogame.game == &ccmgBeStrong && cc->previousMicrogame.game == &ccmgBreakTime)
                || (cc->activeMicrogame.game == &ccmgBreakTime && cc->previousMicrogame.game == &ccmgBeStrong)))
        {
            trophyUpdate(&ccTrophies[UNBOTHERED], 1, true);
        }

        if (successful)
        {
            bool allSuccessful = true;
            for (int i = 0; i < ARRAY_SIZE(microgames); i++)
            {
                if (microgames[i] == cc->activeMicrogame.game)
                {
                    cc->successfulMicrogames[i] = true;
                }
                allSuccessful &= cc->successfulMicrogames[i];
            }

            if (allSuccessful)
            {
                trophyUpdate(&ccTrophies[COMPLETE_EVERY_MICROGAME], 1, true);
            }
        }

        cc->previousMicrogame.game       = cc->activeMicrogame.game;
        cc->previousMicrogame.successful = successful;
    }
}
