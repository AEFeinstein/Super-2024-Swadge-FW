#include "cosCrunch.h"

#include "ccmgBreakTime.h"
#include "ccmgSpray.h"
#include "cosCrunchUtil.h"
#include "mainMenu.h"
#include "swadge2024.h"
#include "wsgPalette.h"

static const char cosCrunchName[]             = "Cosplay Crunch";
static const char cosCrunchStartCraftingLbl[] = "Start Crafting";
static const char cosCrunchExitLbl[]          = "Exit";

static const char cosCrunchGameOverTitle[]   = "Womp, Womp";
static const char cosCrunchGameOverMessage[] = "Press A to retry";

typedef enum
{
    CC_MENU,
    CC_MICROGAME_PENDING,
    CC_MICROGAME_RUNNING,
    CC_GAME_OVER,
} cosCrunchState;

typedef struct
{
    uint8_t lives;
    uint64_t score;
    cosCrunchState state;

    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;

    struct
    {
        cosCrunchMicrogame_t* game;
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
        wsg_t background;
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
} cosCrunch_t;
cosCrunch_t* cc = NULL;

static void cosCrunchEnterMode(void);
static void cosCrunchExitMode(void);
static void cosCrunchMenu(const char* label, bool selected, uint32_t value);
static void cosCrunchMainLoop(int64_t elapsedUs);
static void cosCrunchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void cosCrunchDisplayMessage(const char* msg);
static void cosCrunchDrawTimer(void);

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
};

cosCrunchMicrogame_t* const microgames[] = {
    &ccmgBreakTime,
    &ccmgSpray,
};

#define NUM_LIVES                        4
#define MICROGAME_GET_READY_TIME_US      1000000
#define MICROGAME_RESULT_DISPLAY_TIME_US 1800000
#define TIMER_PIXELS_PER_SECOND          10

#define MESSAGE_X_OFFSET 25
#define MESSAGE_Y_OFFSET 55

tintColor_t const blueTimerTintColor   = {c013, c125, c235};
tintColor_t const yellowTimerTintColor = {c430, c540, c554};
tintColor_t const redTimerTintColor    = {c300, c500, c533};

/// Colors randomly given to games that request a color
tintColor_t tintColors[] = {
    {c010, c232, c343}, // Green
    {c100, c210, c310}, // Brown
    {c134, c245, c355}, // Sky blue
    {c012, c023, c134}, // Aqua
    {c102, c213, c324}, // Violet
    {c304, c415, c525}, // Magenta
    {c440, c551, c554}, // Yellow
    {c420, c530, c541}, // Orange
    {c412, c523, c534}, // Pink
};

static void cosCrunchEnterMode(void)
{
    setFrameRateUs(1000000 / 40);

    cc        = heap_caps_calloc(1, sizeof(cosCrunch_t), MALLOC_CAP_8BIT);
    cc->state = CC_MENU;

    cc->menu = initMenu(cosCrunchName, cosCrunchMenu);
    addSingleItemToMenu(cc->menu, cosCrunchStartCraftingLbl);
    addSingleItemToMenu(cc->menu, cosCrunchExitLbl);
    cc->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    wsgPaletteReset(&cc->tintPalette);
    wsgPaletteReset(&cc->mgTintPalette);
    cc->mgTintColorIndex = -1; // Will be initialized when a microgame requests a tint color

    loadWsg(CC_BACKGROUND_WSG, &cc->wsg.background, true);
    loadWsg(CC_CALENDAR_WSG, &cc->wsg.calendar, false);
    loadWsg(CC_PAINT_LABEL_WSG, &cc->wsg.paintLabel, false);
    loadWsg(CC_PAINT_TUBE_WSG, &cc->wsg.paintTube, false);
    loadWsg(CC_TIMER_LEFT_WSG, &cc->wsg.timerLeft, false);
    loadWsg(CC_TIMER_RIGHT_WSG, &cc->wsg.timerRight, false);

    cc->wsg.backgroundSplatter.w  = TFT_WIDTH;
    cc->wsg.backgroundSplatter.h  = TFT_HEIGHT;
    cc->wsg.backgroundSplatter.px = cc->backgroundSplatterPixels;

    cc->font = *getSysFont();
    loadFont(RIGHTEOUS_150_FONT, &cc->bigFont, false);
    makeOutlineFont(&cc->bigFont, &cc->bigFontOutline, false);
}

static void cosCrunchExitMode(void)
{
    if (cc->activeMicrogame.game != NULL)
    {
        cc->activeMicrogame.game->fnDestroyMicrogame();
    }

    deinitMenuManiaRenderer(cc->menuRenderer);
    deinitMenu(cc->menu);

    freeWsg(&cc->wsg.background);
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
            cc->state = CC_MICROGAME_PENDING;
            // Reset the background splatter for the first/next game
            drawToCanvasTile(cc->wsg.backgroundSplatter, cc->wsg.background, 0, 0);
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
        else if (cc->state == CC_GAME_OVER && (evt.button == PB_A || evt.button == PB_START) && evt.down)
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

            cc->activeMicrogame.game = microgames[esp_random() % (sizeof(microgames) / sizeof(cosCrunchMicrogame_t*))];
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
                            cc->state = CC_GAME_OVER;
                            cc->activeMicrogame.game->fnDestroyMicrogame();
                            cc->activeMicrogame.game = NULL;
                        }
                        else
                        {
                            cc->state = CC_MICROGAME_PENDING;
                        }
                    }
                    break;
            }

            if (cc->activeMicrogame.state == CC_MG_GET_READY)
            {
                cosCrunchDisplayMessage(cc->activeMicrogame.game->verb);
            }

            cosCrunchDrawTimer();

            drawWsgSimple(&cc->wsg.calendar, TFT_WIDTH - cc->wsg.calendar.w - 7, TFT_HEIGHT - cc->wsg.calendar.h - 1);
            char buf[4];
            snprintf(buf, sizeof(buf), "%d", cc->lives);
            uint16_t tw = textWidth(&cc->font, buf);
            drawText(&cc->font, c000, buf, TFT_WIDTH - 25 - tw, TFT_HEIGHT - cc->font.height - 19);

            break;
        }

        case CC_GAME_OVER:
        {
            cosCrunchDisplayMessage(cosCrunchGameOverTitle);

            uint16_t tw = textWidth(&cc->font, cosCrunchGameOverMessage);
            drawText(&cc->font, c555, cosCrunchGameOverMessage, (TFT_WIDTH - tw) / 2, 150);
            break;
        }
    }

    DRAW_FPS_COUNTER(cc->font);
}

static void cosCrunchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    switch (cc->state)
    {
        case CC_MICROGAME_RUNNING:
        case CC_GAME_OVER:
        {
            paletteColor_t* tftFb = getPxTftFramebuffer();
            for (int16_t row = y; row < y + h; row++)
            {
                memcpy(&tftFb[row * TFT_WIDTH + x], &cc->wsg.backgroundSplatter.px[row * TFT_WIDTH + x],
                       w * sizeof(paletteColor_t));
            }
            break;
        }

        case CC_MENU:
        case CC_MICROGAME_PENDING: // Nothing is drawn while loading a microgame, so don't blank the screen then
            break;
    }
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

const tintColor_t* cosCrunchMicrogameGetTintColor()
{
    // Pick a random color that's different from the last one we used
    int16_t tintColorIndex;
    do
    {
        tintColorIndex = esp_random() % (sizeof(tintColors) / sizeof(tintColor_t));
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
