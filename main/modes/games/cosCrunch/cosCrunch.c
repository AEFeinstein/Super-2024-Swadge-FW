#include "cosCrunch.h"

#include "mainMenu.h"
#include "microgameA.h"
#include "microgameB.h"
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

    /// Used to tint grayscale images (c111, c222, c333, c444). See PALETTE_* defines
    wsgPalette_t tintPalette;

    struct
    {
        wsg_t background;
        wsg_t calendar;
        wsg_t paintLabel;
        wsg_t paintTube;
        wsg_t timerLeft;
        wsg_t timerRight;
    } wsg;

    font_t font;
    font_t big_font;
} cosCrunch_t;
cosCrunch_t* cc = NULL;

static void cosCrunchEnterMode(void);
static void cosCrunchExitMode(void);
static void cosCrunchMenu(const char* label, bool selected, uint32_t value);
static void cosCrunchMainLoop(int64_t elapsedUs);
static void cosCrunchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
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
    &ccMicrogameA,
    &ccMicrogameB,
};

#define NUM_LIVES                        4
#define MICROGAME_GET_READY_TIME_US      1000000
#define MICROGAME_RESULT_DISPLAY_TIME_US 1800000
#define TIMER_PIXELS_PER_SECOND          10

// Palette colors used to tint grayscale images. See cosCrunch_t.tintPalette
#define PALETTE_LOWLIGHT  c111
#define PALETTE_BASE      c222
#define PALETTE_HIGHLIGHT c444

static void cosCrunchEnterMode(void)
{
    cc        = heap_caps_calloc(1, sizeof(cosCrunch_t), MALLOC_CAP_8BIT);
    cc->state = CC_MENU;

    cc->menu = initMenu(cosCrunchName, cosCrunchMenu);
    addSingleItemToMenu(cc->menu, cosCrunchStartCraftingLbl);
    addSingleItemToMenu(cc->menu, cosCrunchExitLbl);
    cc->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    wsgPaletteReset(&cc->tintPalette);

    loadWsg(CC_BACKGROUND_WSG, &cc->wsg.background, false);
    loadWsg(CC_CALENDAR_WSG, &cc->wsg.calendar, false);
    loadWsg(CC_PAINT_LABEL_WSG, &cc->wsg.paintLabel, false);
    loadWsg(CC_PAINT_TUBE_WSG, &cc->wsg.paintTube, false);
    loadWsg(CC_TIMER_LEFT_WSG, &cc->wsg.timerLeft, false);
    loadWsg(CC_TIMER_RIGHT_WSG, &cc->wsg.timerRight, false);

    loadFont(IBM_VGA_8_FONT, &cc->font, false);
    loadFont(RIGHTEOUS_150_FONT, &cc->big_font, false);
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

    freeFont(&cc->font);
    freeFont(&cc->big_font);

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

            switch (cc->activeMicrogame.state)
            {
                case CC_MG_GET_READY:
                    if (cc->activeMicrogame.stateElapsedUs >= MICROGAME_GET_READY_TIME_US)
                    {
                        cc->activeMicrogame.state          = CC_MG_PLAYING;
                        cc->activeMicrogame.stateElapsedUs = 0;
                    }
                    else
                    {
                        uint16_t tw = textWidth(&cc->big_font, cc->activeMicrogame.game->verb);
                        drawText(&cc->big_font, c555, cc->activeMicrogame.game->verb, (TFT_WIDTH - tw) / 2, 60);
                    }
                    break;

                case CC_MG_PLAYING:
                    cc->activeMicrogame.gameTimeRemainingUs
                        = CLAMP(cc->activeMicrogame.gameTimeRemainingUs - elapsedUs, 0, INT64_MAX);
                    if (cc->activeMicrogame.gameTimeRemainingUs == 0)
                    {
                        cosCrunchMicrogameResult(false);
                    }
                    break;

                case CC_MG_CELEBRATING:
                case CC_MG_DESPAIRING:
                    if (cc->activeMicrogame.stateElapsedUs >= MICROGAME_RESULT_DISPLAY_TIME_US)
                    {
                        if (cc->lives == 0)
                        {
                            cc->state = CC_GAME_OVER;
                        }
                        else
                        {
                            cc->state = CC_MICROGAME_PENDING;
                        }
                    }
                    break;
            }

            cc->activeMicrogame.game->fnMainLoop(elapsedUs, cc->activeMicrogame.gameTimeRemainingUs,
                                                 cc->activeMicrogame.state, evts, evtCount);

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
            uint16_t tw = textWidth(&cc->big_font, cosCrunchGameOverTitle);
            drawText(&cc->big_font, c555, cosCrunchGameOverTitle, (TFT_WIDTH - tw) / 2, 60);

            tw = textWidth(&cc->font, cosCrunchGameOverMessage);
            drawText(&cc->font, c555, cosCrunchGameOverMessage, (TFT_WIDTH - tw) / 2, 150);
            break;
        }
    }
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
                memcpy(&tftFb[row * TFT_WIDTH + x], &cc->wsg.background.px[row * TFT_WIDTH + x],
                       w * sizeof(paletteColor_t));
            }
            break;
        }

        case CC_MENU:
        case CC_MICROGAME_PENDING: // Nothing is drawn while loading a microgame, so don't blank the screen then
            break;
    }
}

static void cosCrunchDrawTimer()
{
    if (cc->activeMicrogame.gameTimeRemainingUs <= 1000000)
    {
        // Red
        wsgPaletteSet(&cc->tintPalette, PALETTE_LOWLIGHT, c300);
        wsgPaletteSet(&cc->tintPalette, PALETTE_BASE, c500);
        wsgPaletteSet(&cc->tintPalette, PALETTE_HIGHLIGHT, c533);
    }
    else if (cc->activeMicrogame.gameTimeRemainingUs <= cc->activeMicrogame.game->timeoutUs / 2)
    {
        // Yellow
        wsgPaletteSet(&cc->tintPalette, PALETTE_LOWLIGHT, c430);
        wsgPaletteSet(&cc->tintPalette, PALETTE_BASE, c540);
        wsgPaletteSet(&cc->tintPalette, PALETTE_HIGHLIGHT, c554);
    }
    else
    {
        // Blue
        wsgPaletteSet(&cc->tintPalette, PALETTE_LOWLIGHT, c013);
        wsgPaletteSet(&cc->tintPalette, PALETTE_BASE, c125);
        wsgPaletteSet(&cc->tintPalette, PALETTE_HIGHLIGHT, c235);
    }

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
        fillDisplayArea(timerX, timerY, timerX + timer_width, timerY + 4, cc->tintPalette.newColors[PALETTE_BASE]);

        // Paint line highlight, lowlight, shadow
        drawLineFast(timerX, timerY + 1, timerX + timer_width, timerY + 1,
                     cc->tintPalette.newColors[PALETTE_HIGHLIGHT]);
        drawLineFast(timerX, timerY + 4, timerX + timer_width, timerY + 4, cc->tintPalette.newColors[PALETTE_LOWLIGHT]);
        drawLineFast(timerX, timerY + 5, timerX + timer_width, timerY + 5, c210);

        // Round end cap
        drawWsgPaletteSimple(&cc->wsg.timerRight, 66 + timer_width + offsetX,
                             TFT_HEIGHT - cc->wsg.timerRight.h - 11 + offsetY, &cc->tintPalette);
    }

    drawWsgSimple(&cc->wsg.paintTube, 8, TFT_HEIGHT - cc->wsg.paintTube.h - 4);
    drawWsgPaletteSimple(&cc->wsg.paintLabel, 37, TFT_HEIGHT - cc->wsg.paintLabel.h - 8, &cc->tintPalette);
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
