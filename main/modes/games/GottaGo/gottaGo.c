//==============================================================================
// Includes
//==============================================================================

#include "gottaGo.h"
#include "mainMenu.h"

// Data
#include "ggCommonTypes.h"
#include "ggTrophies.h"
#include "ggWSGs.h"
#include "ggText.h"

// Funcs
#include "ggDraw.h"

//==============================================================================
// Defines
//==============================================================================

#define FRAME_RATE_US 16667

#define MAX_TIMER_LEN (GG_SECOND * 10)
#define READY_TIMEOUT (GG_SECOND * 3)

#define HS_PAGE_COUNT 4
/*
#define SOLUTION_TIMER 2000000
 */

//==============================================================================
// Consts
//==============================================================================

const char* const ggModeName = ggName;

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    GG_PLAY_GAME,
    GG_SHOW_RULES,
    GG_SHOW_HS,
    GG_QUIT,
    GG_MENU_OPTIONS
} ggMenuItems_t;

//==============================================================================
// Functions declarations
//==============================================================================

// Main
static void ggEnterMode(void);
static void ggExitMode(void);
static void ggMainLoop(int64_t elapsedUs);

// States
static void doWarning(int64_t elapsedUS);
static void doSplash(int64_t elapsedUs);
static void doMenu(void);
static void doReady(int64_t elapsedUs);
static void doGame();
static void doRules(void);
static void doHS(void);

// Helpers
static void initLevel(void);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t gottaGoMode = {
    .modeName          = ggModeName,
    .wifiMode          = NO_WIFI,
    .overrideUsb       = false,
    .usesAccelerometer = false,
    .usesThermometer   = false,
    .fnEnterMode       = ggEnterMode,
    .fnExitMode        = ggExitMode,
    .fnMainLoop        = ggMainLoop,
    .overrideSelectBtn = true,
    .trophyData        = &ggTrophyDate,
};

ggData_t* ggd;

//==============================================================================
// Functions
//==============================================================================

// Main
static void ggEnterMode(void)
{
    setFrameRateUs(FRAME_RATE_US);
    // Loading resources
    ggd = (ggData_t*)heap_caps_calloc(1, sizeof(ggData_t), MALLOC_CAP_8BIT);
    // Font
    loadFont(PULSE_AUX_FONT, &ggd->titleFont, true);
    makeOutlineFont(&ggd->titleFont, &ggd->titleFontOutline, true);
    loadFont(OXANIUM_13MED_FONT, &ggd->descFont, true);
    // Images
    ggd->backgroundImages = heap_caps_calloc(ARRAY_SIZE(backgroundImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(backgroundImages); idx++)
    {
        loadWsg(backgroundImages[idx], &ggd->backgroundImages[idx], true);
    }
    ggd->urinalImages = heap_caps_calloc(ARRAY_SIZE(urinalImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(urinalImages); idx++)
    {
        loadWsg(urinalImages[idx], &ggd->urinalImages[idx], true);
    }
    ggd->npcImages = heap_caps_calloc(ARRAY_SIZE(npcImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(npcImages); idx++)
    {
        loadWsg(npcImages[idx], &ggd->npcImages[idx], true);
    }
    ggd->uiImages = heap_caps_calloc(ARRAY_SIZE(uiImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
    {
        loadWsg(uiImages[idx], &ggd->uiImages[idx], true);
    }
    ggd->titleImages = heap_caps_calloc(ARRAY_SIZE(titleImages), sizeof(wsg_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(titleImages); idx++)
    {
        loadWsg(titleImages[idx], &ggd->titleImages[idx], true);
    }

    // Initialize
    wsgPaletteReset(&ggd->npcPalette);
    ggInitWarning(ggd);
    ggd->state = GG_WARNING;
}

static void ggExitMode(void)
{
    for (int idx = 0; idx < ARRAY_SIZE(uiImages); idx++)
    {
        freeWsg(&ggd->uiImages[idx]);
    }
    free(ggd->uiImages);
    for (int idx = 0; idx < ARRAY_SIZE(titleImages); idx++)
    {
        freeWsg(&ggd->titleImages[idx]);
    }
    free(ggd->titleImages);
    for (int idx = 0; idx < ARRAY_SIZE(npcImages); idx++)
    {
        freeWsg(&ggd->npcImages[idx]);
    }
    free(ggd->npcImages);
    for (int idx = 0; idx < ARRAY_SIZE(urinalImages); idx++)
    {
        freeWsg(&ggd->urinalImages[idx]);
    }
    free(ggd->urinalImages);
    for (int idx = 0; idx < ARRAY_SIZE(backgroundImages); idx++)
    {
        freeWsg(&ggd->backgroundImages[idx]);
    }
    free(ggd->backgroundImages);
    freeFont(&ggd->descFont);
    freeFont(&ggd->titleFontOutline);
    freeFont(&ggd->titleFont);
    free(ggd);
}

static void ggMainLoop(int64_t elapsedUs)
{
    switch (ggd->state)
    {
        case GG_WARNING:
        {
            doWarning(elapsedUs);
            break;
        }
        case GG_SPLASH:
        {
            doSplash(elapsedUs);
            break;
        }
        case GG_MENU:
        {
            doMenu();
            break;
        }
        case GG_READY:
        {
            doReady(elapsedUs);
            break;
        }
        case GG_RULES:
        {
            doRules();
            break;
        }
        case GG_HIGHSCORE:
        {
            doHS();
            break;
        }
        case GG_GAME:
        {
            doGame();
            break;
        }
        case GG_CHOICE:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                // Speed up timer
                ggd->timer += SECOND;
            }
            ggd->timer += elapsedUs;
            if (ggd->timer >= SOLUTION_TIMER)
            {
                end();
            }
            drawGame(false);
            drawSolution();
            break;
        }
        case GG_WIN:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    ggd->state = GG_READY;
                    ggd->timer = 0;
                }
            }
            drawWin();
            break;
        }
        case GG_LOSE:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    ggd->selection = GG_PLAY_GAME;
                    ggd->state     = GG_MENU;
                }
            }
            drawLose();
            break;
        }
    }
}

static void doWarning(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down && (evt.button & PB_A) && ggd->toggle)
        {
            initSplash();
            ggd->state = GG_SPLASH;
        }
        else if (evt.down)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    ggDrawWarning(ggd, elapsedUs);
}

static void doSplash(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.button & PB_A && evt.down)
        {
            ggd->state = GG_MENU;
        }
    }
    ggDrawSplash(ggd, elapsedUs);
}

static void doMenu()
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_DOWN)
            {
                ggd->selection++;
                ggd->selection %= GG_TEXT_MENU_COUNT + 1;
            }
            if (evt.button & PB_UP)
            {
                ggd->selection--;
                if (ggd->selection < 0)
                {
                    ggd->selection = GG_TEXT_MENU_COUNT;
                }
            }
            if (evt.button & PB_B)
            {
                ggInitSplash(ggd);
                ggd->state = GG_SPLASH;
            }
            if (evt.button & PB_A)
            {
                // Move to appropriate mode
                switch (ggd->selection)
                {
                    case GG_PLAY_GAME:
                    {
                        initLevel();
                        ggd->state = GG_READY;
                        break;
                    }
                    case GG_SHOW_RULES:
                    {
                        ggd->state     = GG_RULES;
                        ggd->selection = 0;
                        break;
                    }
                    case GG_SHOW_HS:
                    {
                        ggd->state     = GG_HIGHSCORE;
                        ggd->selection = 0;
                        break;
                    }
                    case GG_QUIT:
                    {
                        switchToSwadgeMode(&mainMenuMode);
                    }
                    default:
                    {
                        break;
                    }
                }
            }
        }
    }
    ggDrawMenu(ggd);
}

static void doReady(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            ggd->timer += GG_SECOND;
        }
    }
    ggd->timer += elapsedUs;
    if (ggd->timer >= READY_TIMEOUT)
    {
        ggd->timer = 0;
        gameInit();
        ggd->state = GG_GAME;
    }
    drawReady(elapsedUs);
}

static void doGame()
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_A)
            {
                if (ggd->pause)
                {
                    ggd->pause = false;
                }
                else
                {
                    ggd->lose      = ggd->toilets[ggd->selection].brokenBowl == 1
                                     || ggd->toilets[ggd->selection].brokenDrain == 1
                                     || ggd->toilets[ggd->selection].outOfOrder == 1
                                     || ggd->toilets[ggd->selection].pluggedDrain == 1;
                    ggd->stallUsed = false;
                    ggd->state     = GG_CHOICE;
                    ggd->saveTimer = ggd->timer;
                    ggd->timer     = 0;
                }
            }
            else if (evt.button & PB_B)
            {
                if (ggd->pause)
                {
                    ggd->stallUses--;
                    ggd->pause     = false;
                    ggd->lose      = true;
                    ggd->stallUsed = false;
                    ggd->state     = GG_CHOICE;
                    ggd->saveTimer = ggd->timer;
                    ggd->timer     = 0;
                }
                else if (ggd->stallUses > 0)
                {
                    ggd->stallUses--;
                    ggd->lose      = false;
                    ggd->stallUsed = true;
                    ggd->state     = GG_CHOICE;
                    ggd->saveTimer = ggd->timer;
                    ggd->timer     = 0;

                    // Trophy
                    bool trophy = true;
                    for (int idx = 0; idx < ggd->numActive; idx++)
                    {
                        ggToilet_t* t = &ggd->toilets[idx];
                        if (!t->npc.active && !t->brokenBowl && !t->brokenDrain && !t->outOfOrder && !t->pluggedDrain)
                        {
                            trophy = true;
                        }
                    }
                    if (trophy)
                    {
                        trophyUpdate(&ggTrophies[7], 1, true);
                    }
                }
                else
                {
                    ggd->lose      = true;
                    ggd->stallUsed = true;
                    ggd->state     = GG_CHOICE;
                    ggd->saveTimer = ggd->timer;
                    ggd->timer     = 0;
                    trophyUpdate(&ggTrophies[8], 1, true);
                }
            }
            else if (evt.button & PB_LEFT)
            {
                ggd->selection--;
                if (ggd->selection < 0)
                {
                    ggd->selection = ggd->numActive - 1;
                }
                while (ggd->toilets[ggd->selection].npc.active != 0)
                {
                    ggd->selection--;
                    if (ggd->selection < 0)
                    {
                        ggd->selection = ggd->numActive - 1;
                    }
                }
            }
            else if (evt.button & PB_RIGHT)
            {
                ggd->selection++;
                if (ggd->selection >= ggd->numActive)
                {
                    ggd->selection = 0;
                }
                while (ggd->toilets[ggd->selection].npc.active != 0)
                {
                    ggd->selection++;
                    if (ggd->selection >= ggd->numActive)
                    {
                        ggd->selection = 0;
                    }
                }
            }
            else if (evt.button & PB_START || evt.button & PB_SELECT)
            {
                ggd->pause = true;
            }
        }
    }
    linearTouch_t touch[2] = {0};
    getTouchLinear(touch, ARRAY_SIZE(touch));
    if (touch[0].touched)
    {
        int pos      = touch[0].position;
        int numValid = 0;
        for (int idx = 0; idx < ggd->numActive; idx++)
        {
            if (!ggd->toilets[idx].npc.active)
            {
                numValid++;
            }
        }
        int section    = 1024 / numValid;
        int option     = pos / section;
        ggd->selection = 0;
        while (ggd->toilets[ggd->selection].npc.active != 0)
        {
            ggd->selection++;
        }
        while (option > 0)
        {
            ggd->selection++;
            while (ggd->toilets[ggd->selection].npc.active != 0)
            {
                ggd->selection++;
            }
            option--;
        }
    }
    // Timer
    if (ggd->pause)
    {
        drawPause();
    }
    else
    {
        ggd->timer += elapsedUs;
        if (ggd->timer >= ggd->loseTimerMax)
        {
            ggd->lose      = true;
            ggd->stallUsed = false;
            ggd->state     = GG_CHOICE;
            ggd->timer     = 0;
            ggd->timeOut   = true;
        }
        drawGame(true);
    }
}

static void doRules()
{
    buttonEvt_t evt;
    int size = ARRAY_SIZE(rulesText) / 2;
    while (checkButtonQueueWrapper(&evt))
    {
        if (!evt.down)
        {
            if (evt.button & PB_RIGHT)
            {
                ggd->selection++;
                ggd->selection %= size;
            }
            if (evt.button & PB_LEFT)
            {
                ggd->selection--;
                if (ggd->selection < 0)
                {
                    ggd->selection = size - 1;
                }
            }
            if (evt.button & PB_B)
            {
                ggd->selection = GG_SHOW_RULES;
                ggd->state     = GG_MENU;
            }
        }
    }
    ggDrawRules(ggd);
}

static void doHS()
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.button & PB_DOWN)
        {
            ggd->selection++;
            ggd->selection %= HS_PAGE_COUNT;
        }
        if (evt.button & PB_UP)
        {
            ggd->selection--;
            if (ggd->selection < 0)
            {
                ggd->selection = HS_PAGE_COUNT - 1;
            }
        }
        if (evt.button & PB_B)
        {
            ggd->selection = GG_SHOW_HS;
            ggd->state     = GG_MENU;
        }
    }
    ggDrawHighScore(ggd);
}

static void initLevel()
{
    // Main
    ggd->timer     = 0;
    ggd->timeLimit = MAX_TIMER_LEN;
    // Level
    ggd->stallUses = 3;
    ggd->stallReq  = 3;
    ggd->hasLost   = false;
    // Score
    ggd->numLevels  = 0;
    ggd->accScore   = 0;
    ggd->totalScore = 0;
    ggd->adjScore   = 0;
    ggd->stallUsed  = false;
    // Trophies
    ggd->accLevel1 = 0;
    ggd->accLevel2 = 0;
    ggd->accLevel3 = 0;
    ggd->numWorst  = 0;
}