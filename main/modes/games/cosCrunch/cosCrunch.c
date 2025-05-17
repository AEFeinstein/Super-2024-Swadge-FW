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
cosCrunch_t* cosCrunch = NULL;

static void cosCrunchEnterMode(void);
static void cosCrunchExitMode(void);
static void cosCrunchMenu(const char* label, bool selected, uint32_t value);
static void cosCrunchMainLoop(int64_t elapsedUs);
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
    .fnBackgroundDrawCallback = NULL,
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
    cosCrunch        = heap_caps_calloc(1, sizeof(cosCrunch_t), MALLOC_CAP_8BIT);
    cosCrunch->state = CC_MENU;

    cosCrunch->menu = initMenu(cosCrunchName, cosCrunchMenu);
    addSingleItemToMenu(cosCrunch->menu, cosCrunchStartCraftingLbl);
    addSingleItemToMenu(cosCrunch->menu, cosCrunchExitLbl);
    cosCrunch->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

    wsgPaletteReset(&cosCrunch->tintPalette);

    loadWsg(CC_BACKGROUND_WSG, &cosCrunch->wsg.background, false);
    loadWsg(CC_CALENDAR_WSG, &cosCrunch->wsg.calendar, false);
    loadWsg(CC_PAINT_LABEL_WSG, &cosCrunch->wsg.paintLabel, false);
    loadWsg(CC_PAINT_TUBE_WSG, &cosCrunch->wsg.paintTube, false);
    loadWsg(CC_TIMER_LEFT_WSG, &cosCrunch->wsg.timerLeft, false);
    loadWsg(CC_TIMER_RIGHT_WSG, &cosCrunch->wsg.timerRight, false);

    loadFont(IBM_VGA_8_FONT, &cosCrunch->font, false);
    loadFont(RIGHTEOUS_150_FONT, &cosCrunch->big_font, false);
}

static void cosCrunchExitMode(void)
{
    if (cosCrunch->activeMicrogame.game != NULL)
    {
        cosCrunch->activeMicrogame.game->fnDestroyMicrogame();
    }

    deinitMenuManiaRenderer(cosCrunch->menuRenderer);
    deinitMenu(cosCrunch->menu);

    freeWsg(&cosCrunch->wsg.background);
    freeWsg(&cosCrunch->wsg.calendar);
    freeWsg(&cosCrunch->wsg.paintLabel);
    freeWsg(&cosCrunch->wsg.paintTube);
    freeWsg(&cosCrunch->wsg.timerLeft);
    freeWsg(&cosCrunch->wsg.timerRight);

    freeFont(&cosCrunch->font);
    freeFont(&cosCrunch->big_font);

    heap_caps_free(cosCrunch);
}

static void cosCrunchMenu(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (label == cosCrunchStartCraftingLbl)
        {
            cosCrunch->lives = NUM_LIVES;
            cosCrunch->state = CC_MICROGAME_PENDING;
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
        if (cosCrunch->state == CC_MENU)
        {
            cosCrunch->menu = menuButton(cosCrunch->menu, evt);
        }
        else if (cosCrunch->state == CC_GAME_OVER && (evt.button == PB_A || evt.button == PB_START) && evt.down)
        {
            cosCrunch->state = CC_MENU;
        }
    }

    switch (cosCrunch->state)
    {
        case CC_MENU:
            drawMenuMania(cosCrunch->menu, cosCrunch->menuRenderer, elapsedUs);
            break;

        case CC_MICROGAME_PENDING:
            if (cosCrunch->activeMicrogame.game != NULL)
            {
                cosCrunch->activeMicrogame.game->fnDestroyMicrogame();
            }

            cosCrunch->activeMicrogame.game
                = microgames[esp_random() % (sizeof(microgames) / sizeof(cosCrunchMicrogame_t*))];
            cosCrunch->activeMicrogame.game->fnInitMicrogame();

            cosCrunch->activeMicrogame.gameTimeRemainingUs = cosCrunch->activeMicrogame.game->timeoutUs;
            cosCrunch->activeMicrogame.state               = CC_MG_GET_READY;
            cosCrunch->activeMicrogame.stateElapsedUs      = 0;
            cosCrunch->state                               = CC_MICROGAME_RUNNING;
            break;

        case CC_MICROGAME_RUNNING:
        {
            drawWsgTile(&cosCrunch->wsg.background, 0, 0);

            cosCrunch->activeMicrogame.stateElapsedUs += elapsedUs;

            switch (cosCrunch->activeMicrogame.state)
            {
                case CC_MG_GET_READY:
                    if (cosCrunch->activeMicrogame.stateElapsedUs >= MICROGAME_GET_READY_TIME_US)
                    {
                        cosCrunch->activeMicrogame.state          = CC_MG_PLAYING;
                        cosCrunch->activeMicrogame.stateElapsedUs = 0;
                    }
                    else
                    {
                        uint16_t tw = textWidth(&cosCrunch->big_font, cosCrunch->activeMicrogame.game->verb);
                        drawText(&cosCrunch->big_font, c555, cosCrunch->activeMicrogame.game->verb,
                                 (TFT_WIDTH - tw) / 2, 60);
                    }
                    break;

                case CC_MG_PLAYING:
                    cosCrunch->activeMicrogame.gameTimeRemainingUs
                        = CLAMP(cosCrunch->activeMicrogame.gameTimeRemainingUs - elapsedUs, 0, INT64_MAX);
                    if (cosCrunch->activeMicrogame.gameTimeRemainingUs == 0)
                    {
                        cosCrunchMicrogameResult(false);
                    }
                    break;

                case CC_MG_CELEBRATING:
                case CC_MG_DESPAIRING:
                    if (cosCrunch->activeMicrogame.stateElapsedUs >= MICROGAME_RESULT_DISPLAY_TIME_US)
                    {
                        if (cosCrunch->lives == 0)
                        {
                            cosCrunch->state = CC_GAME_OVER;
                        }
                        else
                        {
                            cosCrunch->state = CC_MICROGAME_PENDING;
                        }
                    }
                    break;
            }

            cosCrunch->activeMicrogame.game->fnMainLoop(elapsedUs, cosCrunch->activeMicrogame.gameTimeRemainingUs,
                                                        cosCrunch->activeMicrogame.state, evts, evtCount);

            cosCrunchDrawTimer();

            drawWsgSimple(&cosCrunch->wsg.calendar, TFT_WIDTH - cosCrunch->wsg.calendar.w - 7,
                          TFT_HEIGHT - cosCrunch->wsg.calendar.h - 1);
            char buf[4];
            snprintf(buf, sizeof(buf), "%d", cosCrunch->lives);
            uint16_t tw = textWidth(&cosCrunch->font, buf);
            drawText(&cosCrunch->font, c000, buf, TFT_WIDTH - 25 - tw, TFT_HEIGHT - cosCrunch->font.height - 19);

            break;
        }

        case CC_GAME_OVER:
        {
            drawWsgTile(&cosCrunch->wsg.background, 0, 0);

            uint16_t tw = textWidth(&cosCrunch->big_font, cosCrunchGameOverTitle);
            drawText(&cosCrunch->big_font, c555, cosCrunchGameOverTitle, (TFT_WIDTH - tw) / 2, 60);

            tw = textWidth(&cosCrunch->font, cosCrunchGameOverMessage);
            drawText(&cosCrunch->font, c555, cosCrunchGameOverMessage, (TFT_WIDTH - tw) / 2, 150);
            break;
        }
    }
}

static void cosCrunchDrawTimer()
{
    if (cosCrunch->activeMicrogame.gameTimeRemainingUs <= 1000000)
    {
        // Red
        wsgPaletteSet(&cosCrunch->tintPalette, PALETTE_LOWLIGHT, c300);
        wsgPaletteSet(&cosCrunch->tintPalette, PALETTE_BASE, c500);
        wsgPaletteSet(&cosCrunch->tintPalette, PALETTE_HIGHLIGHT, c533);
    }
    else if (cosCrunch->activeMicrogame.gameTimeRemainingUs <= cosCrunch->activeMicrogame.game->timeoutUs / 2)
    {
        // Yellow
        wsgPaletteSet(&cosCrunch->tintPalette, PALETTE_LOWLIGHT, c430);
        wsgPaletteSet(&cosCrunch->tintPalette, PALETTE_BASE, c540);
        wsgPaletteSet(&cosCrunch->tintPalette, PALETTE_HIGHLIGHT, c554);
    }
    else
    {
        // Green
        wsgPaletteSet(&cosCrunch->tintPalette, PALETTE_LOWLIGHT, c030);
        wsgPaletteSet(&cosCrunch->tintPalette, PALETTE_BASE, c040);
        wsgPaletteSet(&cosCrunch->tintPalette, PALETTE_HIGHLIGHT, c353);
    }

    if (cosCrunch->activeMicrogame.gameTimeRemainingUs > 0)
    {
        uint8_t timer_width = TIMER_PIXELS_PER_SECOND * cosCrunch->activeMicrogame.gameTimeRemainingUs / 1000000.f;

        // These offsets are to animate the paint going back into the tube when the timer nears the end
        int8_t offsetX = MIN(timer_width - cosCrunch->wsg.timerLeft.w, 0);
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
        drawWsgPaletteSimple(&cosCrunch->wsg.timerLeft, 61 + offsetX,
                             TFT_HEIGHT - cosCrunch->wsg.timerLeft.h - 11 + offsetY, &cosCrunch->tintPalette);

        // Long straight paint line
        int16_t timerX = 61 + offsetX + cosCrunch->wsg.timerLeft.w;
        int16_t timerY = TFT_HEIGHT - 17 + offsetY;
        fillDisplayArea(timerX, timerY, timerX + timer_width, timerY + 4,
                        cosCrunch->tintPalette.newColors[PALETTE_BASE]);

        // Paint line highlight, lowlight, shadow
        drawLineFast(timerX, timerY + 1, timerX + timer_width, timerY + 1,
                     cosCrunch->tintPalette.newColors[PALETTE_HIGHLIGHT]);
        drawLineFast(timerX, timerY + 4, timerX + timer_width, timerY + 4,
                     cosCrunch->tintPalette.newColors[PALETTE_LOWLIGHT]);
        drawLineFast(timerX, timerY + 5, timerX + timer_width, timerY + 5, c210);

        // Round end cap
        drawWsgPaletteSimple(&cosCrunch->wsg.timerRight, 66 + timer_width + offsetX,
                             TFT_HEIGHT - cosCrunch->wsg.timerRight.h - 11 + offsetY, &cosCrunch->tintPalette);
    }

    drawWsgSimple(&cosCrunch->wsg.paintTube, 8, TFT_HEIGHT - cosCrunch->wsg.paintTube.h - 4);
    drawWsgPaletteSimple(&cosCrunch->wsg.paintLabel, 37, TFT_HEIGHT - cosCrunch->wsg.paintLabel.h - 8,
                         &cosCrunch->tintPalette);
}

void cosCrunchMicrogameResult(bool successful)
{
    if (cosCrunch->state == CC_MICROGAME_RUNNING && cosCrunch->activeMicrogame.state == CC_MG_PLAYING)
    {
        if (successful)
        {
            cosCrunch->score++;
            cosCrunch->activeMicrogame.state = CC_MG_CELEBRATING;
        }
        else
        {
            cosCrunch->lives--;
            cosCrunch->activeMicrogame.state = CC_MG_DESPAIRING;
        }
        cosCrunch->activeMicrogame.stateElapsedUs = 0;
    }
}
