#include "cosCrunch.h"

#include "ccmgBreakTime.h"
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

    tintColor_t timerTintColor;
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

    midiFile_t menuBgm;
    midiFile_t gameBgm;
    midiFile_t gameOverBgm;
    midiPlayer_t* bgmPlayer;
    midiPlayer_t* sfxPlayer;
    uint32_t gameBgmOriginalTempo;

    highScores_t highScores;
} cosCrunch_t;
cosCrunch_t* cc = NULL;

static void cosCrunchEnterMode(void);
static void cosCrunchExitMode(void);
static bool cosCrunchMenu(const char* label, bool selected, uint32_t value);
static void cosCrunchMainLoop(int64_t elapsedUs);
static void cosCrunchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void cosCrunchResetBackground(void);
static void cosCrunchDisplayMessage(const char* msg);
static void cosCrunchDrawTimer(void);
static void cosCrunchAddToSwadgePassPacket(swadgePassPacket_t* packet);
static int32_t cosCrunchGetSwadgePassHighScore(const swadgePassPacket_t* packet);
static void cosCrunchSetSwadgePassHighScore(swadgePassPacket_t* packet, int32_t highScore);

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
};

/// Uncomment this to play the specified game repeatedly instead of randomizing.
/// Also enables the FPS counter.
// #define DEV_MODE_MICROGAME &ccmgWhatever

const cosCrunchMicrogame_t* const microgames[] = {
    &ccmgBreakTime, &ccmgDelivery, &ccmgSew, &ccmgSlice, &ccmgSpray, &ccmgThread,
};

#define CC_NVS_NAMESPACE      "cc"
#define NVS_KEY_TUTORIAL_SEEN "tutorialSeen"

#define NUM_LIVES                        4
#define MICROGAME_GET_READY_TIME_US      1000000
#define MICROGAME_RESULT_DISPLAY_TIME_US 1800000
#define SPEED_UP_INTERLUDE_TIME_US       1400000
#define PLAYER_INTERLUDE_TIME_US         2500000
#define TIMER_PIXELS_PER_SECOND          10

#define MICROGAMES_BETWEEN_SPEED_UPS 5
#define SPEED_UP_AMOUNT              .08f

#define MESSAGE_X_OFFSET 25
#define MESSAGE_Y_OFFSET 45
#define TEXT_Y_SPACING   5

#define MESSAGE_BOX_MARGIN    15
#define MESSAGE_BOX_PADDING   10
#define GAME_OVER_SCORE_BOX_Y 165

tintColor_t const blueTimerTintColor   = {c013, c125, c235, 0};
tintColor_t const yellowTimerTintColor = {c430, c540, c554, 0};
tintColor_t const redTimerTintColor    = {c300, c500, c533, 0};

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

    loadMidiFile(HD_CREDITS_MID, &cc->menuBgm, true);
    loadMidiFile(CHOWA_RACE_MID, &cc->gameBgm, true);
    loadMidiFile(FAIRY_FOUNTAIN_MID, &cc->gameOverBgm, true);

    cc->bgmPlayer       = globalMidiPlayerGet(MIDI_BGM);
    cc->bgmPlayer->loop = true;
    midiGmOn(cc->bgmPlayer);
    globalMidiPlayerSetVolume(MIDI_BGM, 12);
    globalMidiPlayerPlaySong(&cc->menuBgm, MIDI_BGM);

    cc->sfxPlayer = globalMidiPlayerGet(MIDI_SFX);

    cc->highScores.highScoreCount = 10;
    initHighScores(&cc->highScores, CC_NVS_NAMESPACE);

    list_t swadgePasses = {0};
    getSwadgePasses(&swadgePasses, &cosCrunchMode, true);
    saveHighScoresFromSwadgePass(&cc->highScores, CC_NVS_NAMESPACE, swadgePasses, cosCrunchGetSwadgePassHighScore);
    freeSwadgePasses(&swadgePasses);
}

static void cosCrunchExitMode(void)
{
    if (cc->activeMicrogame.game != NULL)
    {
        cc->activeMicrogame.game->fnDestroyMicrogame();
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

            globalMidiPlayerPlaySong(&cc->gameBgm, MIDI_BGM);
        }
        else if (label == cosCrunchHighScoresLbl)
        {
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
        else if ((cc->state == CC_GAME_OVER || cc->state == CC_HIGH_SCORES)
                 && (evt.button == PB_A || evt.button == PB_B || evt.button == PB_START) && evt.down)
        {
            if (cc->state == CC_GAME_OVER)
            {
                globalMidiPlayerPlaySong(&cc->menuBgm, MIDI_BGM);
            }
            cc->state = CC_MENU;
        }
    }

    if (cc->activeMicrogame.game != NULL)
    {
        if (cc->activeMicrogame.gameTimeRemainingUs <= cc->activeMicrogame.game->timeoutUs / 4)
        {
            cc->timerTintColor = redTimerTintColor;
        }
        else if (cc->activeMicrogame.gameTimeRemainingUs <= cc->activeMicrogame.game->timeoutUs / 2)
        {
            cc->timerTintColor = yellowTimerTintColor;
        }
        else
        {
            cc->timerTintColor = blueTimerTintColor;
        }
    }

    if (cc->state == CC_MICROGAME_PENDING && cc->announcePlayer)
    {
        cc->announcePlayer = false;
        cc->state          = CC_INTERLUDE;
        snprintf(cc->interlude.message, sizeof(cc->interlude.message), cosCrunchInterludePlayerMsg,
                 cc->currentPlayer + 1);
        cc->interlude.timeUs    = PLAYER_INTERLUDE_TIME_US;
        cc->interlude.elapsedUs = 0;
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
                cc->activeMicrogame.game->fnDestroyMicrogame();
                cc->activeMicrogame.game = NULL;
            }

            cosCrunchDisplayMessage(cc->interlude.message);
            cc->interlude.elapsedUs += elapsedUs * cc->timeScale;
            if (cc->interlude.elapsedUs >= cc->interlude.timeUs)
            {
                cc->state = CC_MICROGAME_PENDING;
            }

            break;
        }

        case CC_MICROGAME_PENDING:
            if (cc->activeMicrogame.game != NULL)
            {
                cc->activeMicrogame.game->fnDestroyMicrogame();
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
            } while (cc->activeMicrogame.game != NULL && cc->activeMicrogame.game == newMicrogame);
            cc->activeMicrogame.game = newMicrogame;
#endif
            cc->activeMicrogame.game->fnInitMicrogame();

            cc->activeMicrogame.gameTimeRemainingUs = cc->activeMicrogame.game->timeoutUs;
            cc->activeMicrogame.state               = CC_MG_GET_READY;
            cc->activeMicrogame.stateElapsedUs      = 0;
            cc->state                               = CC_MICROGAME_RUNNING;
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
                                cc->interlude.timeUs    = SPEED_UP_INTERLUDE_TIME_US;
                                cc->interlude.elapsedUs = 0;

                                if (cc->gameBgmOriginalTempo == 0)
                                {
                                    cc->gameBgmOriginalTempo = cc->bgmPlayer->tempo;
                                }
                                cc->bgmPlayer->tempo = cc->gameBgmOriginalTempo / cc->timeScale;
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
            cc->activeMicrogame.game->fnDestroyMicrogame();
            cc->activeMicrogame.game = NULL;

            if (cc->playerCount == 1)
            {
                cc->personalBestAchieved = cc->players[0].score > cc->highScores.userHighScore;
                score_t scores[]         = {{.score = cc->players[0].score, .swadgePassUsername = 0}};
                updateHighScores(&cc->highScores, CC_NVS_NAMESPACE, scores, ARRAY_SIZE(scores));
            }
            else
            {
                cc->personalBestAchieved = false;
            }

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
            drawText(&cc->bigFont, c555, cosCrunchHighScoresLbl, (TFT_WIDTH - tw) / 2, 15);
            drawText(&cc->bigFontOutline, c000, cosCrunchHighScoresLbl, (TFT_WIDTH - tw) / 2, 15);

            int16_t yOff        = 75;
            uint16_t scoreWidth = textWidth(&cc->font, "0000");
            for (int i = 0; i < ARRAY_SIZE(cc->highScores.highScores); i++)
            {
                if (cc->highScores.highScores[i].score > 0)
                {
                    nameData_t username = {0};
                    if (cc->highScores.highScores[i].swadgePassUsername == 0)
                    {
                        username = *getSystemUsername();
                    }
                    else
                    {
                        setUsernameFrom32(&username, cc->highScores.highScores[i].swadgePassUsername);
                    }
                    drawTextEllipsize(&cc->font, c555, username.nameBuffer, 20, yOff,
                                      TFT_WIDTH - 20 * 2 - scoreWidth - 5, false);

                    char buf[16];
                    snprintf(buf, sizeof(buf), "%" PRIi32, cc->highScores.highScores[i].score);
                    tw = textWidth(&cc->font, buf);
                    drawText(&cc->font, c555, buf, TFT_WIDTH - tw - 20, yOff);
                    yOff += cc->font.height + TEXT_Y_SPACING;
                }
            }

            break;
        }
    }

    // Tempo resets when the BGM loops, so we need to write the tempo every frame while the game is active
    if ((cc->state == CC_MICROGAME_PENDING || cc->state == CC_MICROGAME_RUNNING || cc->state == CC_INTERLUDE)
        && cc->gameBgmOriginalTempo != 0)
    {
        cc->bgmPlayer->tempo = cc->gameBgmOriginalTempo / cc->timeScale;
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

static void cosCrunchDrawTimer()
{
    tintPalette(&cc->tintPalette, &cc->timerTintColor);

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
        fillDisplayArea(timerX, timerY, timerX + timer_width, timerY + 4, cc->timerTintColor.base);

        // Paint line highlight, lowlight, shadow
        drawLineFast(timerX, timerY + 1, timerX + timer_width, timerY + 1, cc->timerTintColor.highlight);
        drawLineFast(timerX, timerY + 4, timerX + timer_width, timerY + 4, cc->timerTintColor.lowlight);
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
        }
        else
        {
            cc->players[cc->currentPlayer].lives--;
            cc->activeMicrogame.state = CC_MG_DESPAIRING;
        }
        cc->activeMicrogame.stateElapsedUs = 0;
    }
}
