#include "cosCrunch.h"

#include "ccmgBreakTime.h"
#include "ccmgDelivery.h"
#include "ccmgSpray.h"
#include "ccmgThread.h"
#include "cosCrunchUtil.h"
#include "highScores.h"
#include "mainMenu.h"
#include "nameList.h"
#include "swadge2024.h"
#include "wsgPalette.h"

static const char cosCrunchName[]             = "Cosplay Crunch";
static const char cosCrunchStartCraftingLbl[] = "Start Crafting";
static const char cosCrunchHighScoresLbl[]    = "High Scores";
static const char cosCrunchExitLbl[]          = "Exit";

static const char cosCrunchGameOverTitle[]   = "Your costumes aren't done!";
static const char cosCrunchYourScoreMsg[]    = "Your score: %" PRIi32;
static const char cosCrunchNewHighScoreMsg[] = "New personal best!";

typedef enum
{
    /// Main menu
    CC_MENU,
    /// A microgame is currently loading: don't blank the display or draw anything
    CC_MICROGAME_PENDING,
    /// A microgame is in progress
    CC_MICROGAME_RUNNING,
    /// The player has lost, but we still need to unload the last microgame played
    CC_GAME_OVER_PENDING,
    /// Game over screen
    CC_GAME_OVER,
    /// High scores screen
    CC_HIGH_SCORES,
} cosCrunchState;

typedef struct
{
    uint8_t lives;
    int32_t score;
    bool personalBestAchieved;
    cosCrunchState state;

    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;

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
    } wsg;
    paletteColor_t backgroundSplatterPixels[TFT_WIDTH * TFT_HEIGHT];

    font_t font;
    font_t bigFont;
    font_t bigFontOutline;

    highScores_t highScores;
} cosCrunch_t;
cosCrunch_t* cc = NULL;

static void cosCrunchEnterMode(void);
static void cosCrunchExitMode(void);
static void cosCrunchMenu(const char* label, bool selected, uint32_t value);
static void cosCrunchMainLoop(int64_t elapsedUs);
static void cosCrunchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void cosCrunchResetBackground(void);
static void cosCrunchDisplayMessage(const char* msg);
static void cosCrunchDrawTimer(void);
static void cosCrunchAddToSwadgePassPacket(struct swadgePassPacket* packet);

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
    &ccmgBreakTime,
    &ccmgDelivery,
    &ccmgSpray,
    &ccmgThread,
};

#define CC_NVS_NAMESPACE "cc"

#define NUM_LIVES                        4
#define MICROGAME_GET_READY_TIME_US      1000000
#define MICROGAME_RESULT_DISPLAY_TIME_US 1800000
#define TIMER_PIXELS_PER_SECOND          10

#define MESSAGE_X_OFFSET 25
#define MESSAGE_Y_OFFSET 45
#define TEXT_Y_SPACING   5

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
    setFrameRateUs(1000000 / 40);

    cc        = heap_caps_calloc(1, sizeof(cosCrunch_t), MALLOC_CAP_8BIT);
    cc->state = CC_MENU;

    cc->menu = initMenu(cosCrunchName, cosCrunchMenu);
    addSingleItemToMenu(cc->menu, cosCrunchStartCraftingLbl);
    addSingleItemToMenu(cc->menu, cosCrunchHighScoresLbl);
    addSingleItemToMenu(cc->menu, cosCrunchExitLbl);
    cc->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

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

    cc->wsg.backgroundSplatter.w  = TFT_WIDTH;
    cc->wsg.backgroundSplatter.h  = TFT_HEIGHT;
    cc->wsg.backgroundSplatter.px = cc->backgroundSplatterPixels;
    cosCrunchResetBackground();

    cc->font = *getSysFont();
    loadFont(RIGHTEOUS_150_FONT, &cc->bigFont, false);
    makeOutlineFont(&cc->bigFont, &cc->bigFontOutline, false);

    cc->highScores.highScoreCount = 10;
    initHighScores(&cc->highScores, CC_NVS_NAMESPACE);

    list_t swadgePasses = {0};
    getSwadgePasses(&swadgePasses, &cosCrunchMode, true);
    SAVE_HIGH_SCORES_FROM_SWADGE_PASS(&cc->highScores, CC_NVS_NAMESPACE, swadgePasses, cosCrunch.highScore);
    freeSwadgePasses(&swadgePasses);
}

static void cosCrunchExitMode(void)
{
    if (cc->activeMicrogame.game != NULL)
    {
        cc->activeMicrogame.game->fnDestroyMicrogame();
    }

    deinitMenuManiaRenderer(cc->menuRenderer);
    deinitMenu(cc->menu);

    freeWsg(&cc->wsg.backgroundMat);
    freeWsg(&cc->wsg.backgroundDesk);
    freeWsg(&cc->wsg.calendar);
    freeWsg(&cc->wsg.paintLabel);
    freeWsg(&cc->wsg.paintTube);
    freeWsg(&cc->wsg.timerLeft);
    freeWsg(&cc->wsg.timerRight);

    freeFont(&cc->bigFont);
    freeFont(&cc->bigFontOutline);

    heap_caps_free(cc);
}

static void cosCrunchMenu(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (label == cosCrunchStartCraftingLbl)
        {
            cc->lives = NUM_LIVES;
            cc->score = 0;
            cc->state = CC_MICROGAME_PENDING;
            // Reset the background splatter for the next game
            cosCrunchResetBackground();
        }
        else if (label == cosCrunchHighScoresLbl)
        {
            cc->state = CC_HIGH_SCORES;
        }
        else if (label == cosCrunchExitLbl)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
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
        else if ((cc->state == CC_GAME_OVER || cc->state == CC_HIGH_SCORES)
                 && (evt.button == PB_A || evt.button == PB_B || evt.button == PB_START) && evt.down)
        {
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

    switch (cc->state)
    {
        case CC_MENU:
            drawMenuMania(cc->menu, cc->menuRenderer, elapsedUs);
            break;

        case CC_MICROGAME_PENDING:
            if (cc->activeMicrogame.game != NULL)
            {
                cc->activeMicrogame.game->fnDestroyMicrogame();
            }

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
            cc->activeMicrogame.stateElapsedUs += elapsedUs;
            cc->activeMicrogame.game->fnMainLoop(elapsedUs, cc->activeMicrogame.gameTimeRemainingUs,
                                                 cc->activeMicrogame.state, evts, evtCount);

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
                        = MAX(cc->activeMicrogame.gameTimeRemainingUs - elapsedUs, 0);
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

                    if (cc->activeMicrogame.stateElapsedUs >= MICROGAME_RESULT_DISPLAY_TIME_US)
                    {
                        if (cc->lives == 0)
                        {
                            cc->state = CC_GAME_OVER_PENDING;
                        }
                        else
                        {
                            cc->state = CC_MICROGAME_PENDING;
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
            snprintf(buf, sizeof(buf), "%d", cc->lives);
            uint16_t tw = textWidth(&cc->font, buf);
            drawText(&cc->font, c000, buf, TFT_WIDTH - 25 - tw, TFT_HEIGHT - cc->font.height - 19);

            break;
        }

        case CC_GAME_OVER_PENDING:
        {
            cc->state = CC_GAME_OVER;
            cc->activeMicrogame.game->fnDestroyMicrogame();
            cc->activeMicrogame.game = NULL;

            cc->personalBestAchieved = cc->score > cc->highScores.userHighScore;

            score_t scores[] = {{.score = cc->score, .swadgePassUsername = 0}};
            updateHighScores(&cc->highScores, CC_NVS_NAMESPACE, scores, ARRAY_SIZE(scores));
            break;
        }

        case CC_GAME_OVER:
        {
            cosCrunchDisplayMessage(cosCrunchGameOverTitle);

            int16_t yOff = 165;
            char buf[32];
            snprintf(buf, sizeof(buf), cosCrunchYourScoreMsg, cc->score);
            uint16_t tw = textWidth(&cc->font, buf);
            drawText(&cc->font, c555, buf, (TFT_WIDTH - tw) / 2, yOff);
            yOff += cc->font.height + TEXT_Y_SPACING;

            if (cc->personalBestAchieved)
            {
                tw = textWidth(&cc->font, cosCrunchNewHighScoreMsg);
                drawText(&cc->font, c555, cosCrunchNewHighScoreMsg, (TFT_WIDTH - tw) / 2, yOff);
            }

            break;
        }

        case CC_HIGH_SCORES:
        {
            uint16_t tw = textWidth(&cc->bigFont, cosCrunchHighScoresLbl);
            drawText(&cc->bigFont, c555, cosCrunchHighScoresLbl, (TFT_WIDTH - tw) / 2, 15);
            drawText(&cc->bigFontOutline, c000, cosCrunchHighScoresLbl, (TFT_WIDTH - tw) / 2, 15);

            int16_t yOff = 75;
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
                    drawText(&cc->font, c555, username.nameBuffer, 25, yOff);

                    char buf[16];
                    snprintf(buf, sizeof(buf), "%" PRIi32, cc->highScores.highScores[i].score);
                    tw = textWidth(&cc->font, buf);
                    drawText(&cc->font, c555, buf, TFT_WIDTH - tw - 25, yOff);
                    yOff += cc->font.height + TEXT_Y_SPACING;
                }
            }

            break;
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
        case CC_MICROGAME_RUNNING:
        case CC_GAME_OVER:
        case CC_HIGH_SCORES:
        {
            if (cc->activeMicrogame.game != NULL && cc->activeMicrogame.game->fnBackgroundDrawCallback != NULL)
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
                for (int16_t row = y; row < y + h; row++)
                {
                    memcpy(&tftFb[row * TFT_WIDTH + x], &cc->wsg.backgroundSplatter.px[row * TFT_WIDTH + x],
                           w * sizeof(paletteColor_t));
                }
            }
            break;
        }

        case CC_MENU:
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

static void cosCrunchAddToSwadgePassPacket(struct swadgePassPacket* packet)
{
    WRITE_HIGH_SCORE_TO_SWADGE_PASS_PACKET(CC_NVS_NAMESPACE, packet->cosCrunch.highScore);
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
        if (successful)
        {
            cc->score++;
            cc->activeMicrogame.state = CC_MG_CELEBRATING;
        }
        else
        {
            cc->lives--;
            cc->activeMicrogame.state = CC_MG_DESPAIRING;
        }
        cc->activeMicrogame.stateElapsedUs = 0;
    }
}
