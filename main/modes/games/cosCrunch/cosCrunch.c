#include "cosCrunch.h"

#include "mainMenu.h"
#include "microgameA.h"
#include "microgameB.h"
#include "swadge2024.h"

static const char cosCrunchName[]             = "Cosplay Crunch";
static const char cosCrunchStartCraftingLbl[] = "Start Crafting";
static const char cosCrunchExitLbl[]          = "Exit";

static const char cosCrunchTimeFormat[]  = "Time: %0.2f";
static const char cosCrunchLivesFormat[] = "Days 'Til MAG: %d";

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

    font_t font;
    font_t big_font;
} cosCrunch_t;
cosCrunch_t* cosCrunch = NULL;

static void cosCrunchEnterMode(void);
static void cosCrunchExitMode(void);
static void cosCrunchMenu(const char* label, bool selected, uint32_t value);
static void cosCrunchMainLoop(int64_t elapsedUs);
static void cosCrunchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

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
#define UI_TEXT_MARGIN                   15

static void cosCrunchEnterMode(void)
{
    cosCrunch        = heap_caps_calloc(1, sizeof(cosCrunch_t), MALLOC_CAP_8BIT);
    cosCrunch->state = CC_MENU;

    cosCrunch->menu = initMenu(cosCrunchName, cosCrunchMenu);
    addSingleItemToMenu(cosCrunch->menu, cosCrunchStartCraftingLbl);
    addSingleItemToMenu(cosCrunch->menu, cosCrunchExitLbl);
    cosCrunch->menuRenderer = initMenuManiaRenderer(NULL, NULL, NULL);

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

            cosCrunch->activeMicrogame.gameTimeRemainingUs = cosCrunch->activeMicrogame.game->timeoutSeconds * 1000000;
            cosCrunch->activeMicrogame.state               = CC_MG_GET_READY;
            cosCrunch->activeMicrogame.stateElapsedUs      = 0;
            cosCrunch->state                               = CC_MICROGAME_RUNNING;
            break;

        case CC_MICROGAME_RUNNING:
        {
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

            char buf[19];
            snprintf(buf, sizeof(buf), cosCrunchTimeFormat, cosCrunch->activeMicrogame.gameTimeRemainingUs / 1000000.f);
            drawText(&cosCrunch->font, c333, buf, UI_TEXT_MARGIN, TFT_HEIGHT - cosCrunch->font.height - UI_TEXT_MARGIN);

            snprintf(buf, sizeof(buf), cosCrunchLivesFormat, cosCrunch->lives);
            uint16_t tw = textWidth(&cosCrunch->font, buf);
            drawText(&cosCrunch->font, c333, buf, TFT_WIDTH - tw - UI_TEXT_MARGIN,
                     TFT_HEIGHT - cosCrunch->font.height - UI_TEXT_MARGIN);

            break;
        }

        case CC_GAME_OVER:
        {
            uint16_t tw = textWidth(&cosCrunch->big_font, cosCrunchGameOverTitle);
            drawText(&cosCrunch->big_font, c555, cosCrunchGameOverTitle, (TFT_WIDTH - tw) / 2, 60);

            tw   = textWidth(&cosCrunch->font, cosCrunchGameOverMessage);
            drawText(&cosCrunch->font, c555, cosCrunchGameOverMessage, (TFT_WIDTH - tw) / 2, 150);
            break;
        }
    }
}

static void cosCrunchBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Nothing is drawn while loading a microgame, so don't blank the screen then
    if (cosCrunch->state != CC_MICROGAME_PENDING)
    {
        fillDisplayArea(x, y, x + w, y + h, c000);
    }
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
