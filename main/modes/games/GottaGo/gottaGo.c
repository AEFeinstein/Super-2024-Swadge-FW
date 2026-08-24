//==============================================================================
// Includes
//==============================================================================

#include "gottaGo.h"
#include "mainMenu.h"

// Data
#include "ggCommonTypes.h"
#include "ggWSGs.h"

// Funcs
#include "ggDraw.h"
#include "ggLevel.h"
#include "ggScoring.h"

//==============================================================================
// Defines
//==============================================================================

#define FRAME_RATE_US 16667

// Timers
#define MAX_TIMER_LEN  (GG_SECOND * 10)
#define READY_TIMEOUT  (GG_SECOND * 3)
#define SOLUTION_TIMER (GG_SECOND * 3)

// High scores
#define HS_PAGE_COUNT 4

// Scores
#define OHP_SCORE 1000
#define NNP_SCORE 990
#define NFP_SCORE 950

// Milestone percents
#define MILE_ACC    33
#define MILE_ROUNDS 25
#define MILE_WORST  99
#define MILE_CASUAL 25

//==============================================================================
// Consts
//==============================================================================

const char ggModeName[] = "Gotta Go!";

const trophyData_t ggTrophies[] = {
    {
        .title       = "Maybe talk to a doctor",
        .description = "Complete 20 rounds of Gotta Go!",
        .image       = GG_20GAMES_WSG,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 20,
    },
    {
        .title       = "Drank a Bit Too Much",
        .description = "Beat 10 levels in a row",
        .image       = GG_10ROUNDS_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 10,
    },
    {
        .title       = "Hydrohomie",
        .description = "Beat 20 levels in a row",
        .image       = GG_20ROUNDS_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 20,
    },
    {
        .title       = "Hyperhydrosis",
        .description = "Beat 30 levels in a row",
        .image       = GG_30ROUNDS_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 30,
    },
    {
        .title       = "Laser beam",
        .description = "Beat 15+ levels with a perfect accuracy",
        .image       = GG_ONE_HUNDRED_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 15,
    },
    {
        .title       = "Sniper",
        .description = "Beat 15+ levels with 99+%% accuracy",
        .image       = GG_NINETY_NINE_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 15,
    },
    {
        .title       = "Sharpshooter",
        .description = "Beat 15+ levels with 95+%% accuracy",
        .image       = GG_NINETY_FIVE_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 15,
    },
    {
        .title       = "Emergency Plan",
        .description = "When all options are filled or broken, use a stall",
        .image       = GG_STALL_CORRECT_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
    },
    {
        .title       = "Stall Hogger",
        .description = "Attempt to use a stall with no uses left",
        .image       = GG_STALL_INCORRECT_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Relaxed pee",
        .description = "Play 100 total levels of casual",
        .image       = GG_20GAMES_WSG,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 100,
    },
    {
        .title       = "Socialite",
        .description = "Pick the worst available option 5 times in a row",
        .image       = GG_WORST_OPTION_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 5,
    },
    {
        .title       = "Aim at the dot",
        .description = "Activate helper mode for the first time",
        .image       = GG_HELPER_MODE_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "DOWN WITH THE ESRB",
        .description = "Find the manifesto",
        .image       = GG_MANI_PEDI_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = true,
    },
    {
        .title       = "The Truth is out there",
        .description = "Learn the truth about the Adraxians",
        .image       = TROPHY_DIFF_EASY,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = true,
    },
};

const trophySettings_t ggTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = ggModeName,
};

const trophyDataList_t ggTrophyDate = {
    .length   = ARRAY_SIZE(ggTrophies),
    .list     = ggTrophies,
    .settings = &ggTrophySettings,
};

//==============================================================================
// Enums
//==============================================================================

/// @brief Trophy names
typedef enum
{
    T_ROUNDS_20,
    T_LEVELS_10,
    T_LEVELS_20,
    T_LEVELS_30,
    T_OHP,
    T_NNP,
    T_NFP,
    T_USE_STALL_GOOD,
    T_USE_STALL_BAD,
    T_CASUAL,
    T_WORST_OPTIONS,
    T_HELP_MODE,
    T_MANIFESTO,
    T_TRUTH,
} ggTrophyNames_t;

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
static void doPickGame(void);
static void doReady(int64_t elapsedUs);
static void doGame(int64_t elapsedUs);
static void doChoice(int64_t elapsedUs);
static void doRules(void);
static void doHS(void);
static void doOptions(void);

// Helpers

/**
 * @brief Initializes a new level
 *
 */
static void initLevel(void);

/**
 * @brief Get the Touch Input
 *
 */
static void getTouchInput(void);

/**
 * @brief Handles the end of the game. Saves score, updates NVS, etc.
 *
 * @param lose If this match resulted in a loss
 * @param stall If a stall was used
 */
static void handleGameEnd(bool lose, bool stall);

/**
 * @brief Checks the accuracy and worst trophies. Requires ggd->selection to remain accurate
 *
 * @param urinalScores Array of scores, one for each urinal in order from left to right
 * @param accuracy Current accuracy
 * @param worst Current worst option value
 */
static void checkTrophies(int* urinalScores, int accuracy, int worst);

/**
 * @brief Checks an accuracy Trophy
 *
 * @param accuracy Current Accuracy
 * @param count Current numer of Levels
 * @param trophyName The trophy to update
 */
static void checkAccuracy(int accuracy, int target, int count, ggTrophyNames_t trophyName);

/**
 * @brief Gets the length of the drink water phrase list
 *
 */
static void drinkWater(void);

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
    int outVal = 0;
    readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_HELPER], &outVal);
    ggd->helper = outVal;
    readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_TOUCH], &outVal);
    ggd->touch = outVal;
    readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_WARNING_NVS], &outVal);
    ggd->toggle = outVal;

    drinkWater();
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
        case GG_PICK_GAME:
        {
            doPickGame();
            break;
        }
        case GG_GAME:
        case GG_CASUAL:
        {
            doGame(elapsedUs);
            break;
        }
        case GG_CHOICE:
        {
            doChoice(elapsedUs);
            break;
        }
        case GG_WON:
        {
            buttonEvt_t evt;
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (ggd->casual)
                    {
                        ggLevelReset(ggd);
                        ggd->state = GG_CASUAL;
                    }
                    else
                    {
                        ggd->state = GG_READY;
                        ggd->timer = 0;
                    }
                }
            }
            ggDrawResult(ggd);
            break;
        }
        case GG_LOST:
        {
            buttonEvt_t evt;
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (ggd->casual)
                    {
                        ggLevelReset(ggd);
                        ggd->state = GG_CASUAL;
                    }
                    else
                    {
                        ggd->pause     = false;
                        ggd->selection = GG_TEXT_MENU_PLAY;
                        ggd->state     = GG_MENU;
                    }
                }
            }
            ggDrawResult(ggd);
            break;
        }
        case GG_OPTIONS:
        {
            doOptions();
        }
    }
}

// States

static void doWarning(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (!ggd->toggle)
        {
        }
        else if (evt.down && (evt.button & PB_A))
        {
            ggInitSplash(ggd);
            writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_WARNING_NVS], true);
            ggd->state = GG_SPLASH;
        }
        else if (evt.down)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    ggDrawWarning(ggd, elapsedUs);
    if (ggd->timer >= LONG_WAIT)
    {
        trophyUpdate(&ggTrophies[T_MANIFESTO], 1, true);
    }
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
                    case GG_TEXT_MENU_PLAY:
                    {
                        initLevel();
                        ggd->state = GG_PICK_GAME;
                        break;
                    }
                    case GG_TEXT_MENU_RULES:
                    {
                        // Set up urinal array for display
                        ggClearUrinals(ggd);
                        ggd->state     = GG_RULES;
                        ggd->selection = 0;
                        break;
                    }
                    case GG_TEXT_MENU_HS:
                    {
                        ggd->state     = GG_HIGHSCORE;
                        ggd->selection = 0;
                        break;
                    }

                    case GG_TEXT_MENU_OPTIONS:
                    {
                        ggd->state     = GG_OPTIONS;
                        ggd->selection = 0;
                        break;
                    }
                    default:
                    {
                        switchToSwadgeMode(&mainMenuMode);
                        break;
                    }
                }
            }
            if (evt.button & PB_START)
            {
                ggd->textOrder++;
                ggd->textOrder = ggd->textOrder % (ggGetDrinkTextLen() + 1);
            }
        }
    }
    ggDrawMenu(ggd);
}

static void doPickGame()
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_B)
            {
                ggd->selection = 0;
                ggd->state     = GG_MENU;
            }
            else if (evt.button & PB_A)
            {
                if (ggd->selection)
                {
                    ggLevelReset(ggd);
                    ggd->stallUses = 9999;
                    ggd->casual    = true;
                    ggd->state     = GG_CASUAL;
                }
                else
                {
                    ggd->casual = false;
                    ggd->state  = GG_READY;
                }
            }
            else if (evt.button & PB_UP)
            {
                ggd->selection = 0;
            }
            else if (evt.button & PB_DOWN)
            {
                ggd->selection = 1;
            }
        }
    }
    ggDrawPick(ggd);
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
        ggLevelReset(ggd);
        ggd->state = GG_GAME;
    }
    ggDrawReady(ggd, elapsedUs);
}

static void doGame(int64_t elapsedUs)
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
                    handleGameEnd(ggd->urinals[ggd->selection].brokenBowl || ggd->urinals[ggd->selection].brokenDrain
                                      || ggd->urinals[ggd->selection].outOfOrder
                                      || ggd->urinals[ggd->selection].pluggedDrain,
                                  false);
                }
            }
            else if (evt.button & PB_B)
            {
                if (ggd->pause)
                {
                    handleGameEnd(true, false);
                }
                else if (ggd->stallUses > 0)
                {
                    ggd->stallUses--;
                    handleGameEnd(false, true);
                    // Trophy
                    bool trophy = true;
                    for (int idx = 0; idx < ggd->numActive; idx++)
                    {
                        ggUrinal_t* u = &ggd->urinals[idx];
                        if (!u->npc.active && !u->brokenBowl && !u->brokenDrain && !u->outOfOrder && !u->pluggedDrain)
                        {
                            trophy = true;
                        }
                    }
                    if (trophy)
                    {
                        trophyUpdate(&ggTrophies[T_USE_STALL_GOOD], 1, true);
                    }
                }
                else
                {
                    handleGameEnd(true, true);
                    trophyUpdate(&ggTrophies[T_USE_STALL_BAD], 1, true);
                }
            }
            else if (evt.button & PB_LEFT)
            {
                ggd->selection--;
                if (ggd->selection < 0)
                {
                    ggd->selection = ggd->numActive - 1;
                }
                while (ggd->urinals[ggd->selection].npc.active != 0)
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
                while (ggd->urinals[ggd->selection].npc.active != 0)
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
    if (ggd->touch)
    {
        getTouchInput();
    }
    if (ggd->pause)
    {
        ggDrawPause(ggd);
    }
    else if (ggd->casual)
    {
        ggDrawLevel(ggd, false);
    }
    else
    {
        ggd->timer += elapsedUs;
        if (ggd->timer >= ggd->timeLimit)
        {
            handleGameEnd(true, false);
        }
        ggDrawLevel(ggd, false);
    }
}

static void doChoice(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // Speed up timer
        ggd->timer += GG_SECOND;
    }
    ggd->timer += elapsedUs;
    if (ggd->timer >= SOLUTION_TIMER)
    {
        // Determine if the player gets another use of the stall
        if (ggd->numLevels > ggd->stallReq)
        {
            ggd->stallUses++;
            ggd->stallReq *= 2;
            ggd->stallReq += ggd->stallReq / 2;
        }
        // Calculate scores
        if (!ggd->hasLost)
        {
            ggd->state = GG_WON;
        }
        else
        {
            ggd->state = GG_LOST;
        }
        // Update trophies
        trophyUpdateMilestone(&ggTrophies[T_LEVELS_10], ggd->numLevels, 50);
        trophyUpdateMilestone(&ggTrophies[T_LEVELS_20], ggd->numLevels, 50);
        trophyUpdateMilestone(&ggTrophies[T_LEVELS_30], ggd->numLevels, 50);
        if (ggd->hasLost)
        {
            trophyUpdateMilestone(&ggTrophies[T_ROUNDS_20], trophyGetSavedValue(&ggTrophies[T_ROUNDS_20]) + 1,
                                  MILE_ROUNDS);
        }
    }
    ggDrawLevel(ggd, true);
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
                ggClearUrinals(ggd);
                ggd->selection++;
                ggd->selection %= size;
            }
            if (evt.button & PB_LEFT)
            {
                ggClearUrinals(ggd);
                ggd->selection--;
                if (ggd->selection < 0)
                {
                    ggd->selection = size - 1;
                }
            }
            if (evt.button & PB_B)
            {
                ggd->selection = GG_TEXT_MENU_RULES;
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
            ggd->selection = GG_TEXT_MENU_HS;
            ggd->state     = GG_MENU;
        }
    }
    ggDrawHighScore(ggd);
}

static void doOptions()
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_DOWN)
            {
                ggd->selection++;
                ggd->selection %= GG_TEXT_OPTIONS_COUNT - GG_TEXT_MENU_COUNT + 1;
            }
            else if (evt.button & PB_UP)
            {
                ggd->selection--;
                if (ggd->selection < 0)
                {
                    ggd->selection = GG_TEXT_OPTIONS_COUNT - GG_TEXT_MENU_COUNT;
                }
            }
            else if (evt.button & PB_LEFT)
            {
                if (ggd->selection == 0)
                {
                    ggd->helper = 0;
                }
                else if (ggd->selection == 1)
                {
                    ggd->touch = 0;
                }
            }
            else if (evt.button & PB_RIGHT)
            {
                if (ggd->selection == 0)
                {
                    ggd->helper = 1;
                    trophyUpdate(&ggTrophies[T_HELP_MODE], 1, true);
                }
                else if (ggd->selection == 1)
                {
                    ggd->touch = 1;
                }
            }
            else if (evt.button & PB_B || ((evt.button & PB_A) && ggd->selection == 2))
            {
                ggd->state     = GG_MENU;
                ggd->selection = GG_TEXT_MENU_OPTIONS;
                writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_HELPER], ggd->helper);
                writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_TOUCH], ggd->touch);
            }
        }
    }
    // Draw
    ggDrawOptions(ggd);
}

// Helpers

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

static void getTouchInput()
{
    linearTouch_t touch[2] = {0};
    getTouchLinear(touch, ARRAY_SIZE(touch));
    if (touch[0].touched)
    {
        int pos      = touch[0].position;
        int numValid = 0;
        for (int idx = 0; idx < ggd->numActive; idx++)
        {
            if (!ggd->urinals[idx].npc.active)
            {
                numValid++;
            }
        }
        int section    = 1024 / numValid;
        int option     = pos / section;
        ggd->selection = 0;
        while (ggd->urinals[ggd->selection].npc.active)
        {
            ggd->selection++;
        }
        while (option > 0)
        {
            ggd->selection++;
            while (ggd->urinals[ggd->selection].npc.active)
            {
                ggd->selection++;
            }
            option--;
        }
    }
}

static void handleGameEnd(bool lose, bool stall)
{
    // Set values
    ggd->hasLost       = lose;
    ggd->stallUsed     = stall;
    ggd->timeRemaining = ggd->timeLimit - ggd->timer;
    ggd->numLevels++;

    int final[MAX_URINALS] = {0};
    int best, worst;
    if (!ggd->pause)
    {
        ggCalcFinalScores(ggd, final, &best, &worst);
    }
    if (!ggd->casual)
    {
        checkTrophies(final, ggd->accScore, worst);
    } else {
        trophyUpdateMilestone(&ggTrophies[T_WORST_OPTIONS], ggd->numLevels, MILE_CASUAL);
    }

    // Reset
    ggd->timer = 0;
    ggd->state = GG_CHOICE;
}

static void checkTrophies(int* urinalScores, int accuracy, int worst)
{
    // Reset accuracy trophy trackers if not accurate enough
    checkAccuracy(accuracy, OHP_SCORE, ggd->accLevel1, T_OHP);
    checkAccuracy(accuracy, NNP_SCORE, ggd->accLevel2, T_NNP);
    checkAccuracy(accuracy, NFP_SCORE, ggd->accLevel3, T_NFP);

    if (urinalScores[ggd->selection] == worst)
    {
        ggd->numWorst++;
        trophyUpdateMilestone(&ggTrophies[T_CASUAL], ggd->numWorst, MILE_WORST);
    }
    else
    {
        ggd->numWorst = 0;
    }
}

static void checkAccuracy(int accuracy, int target, int count, ggTrophyNames_t trophyName)
{
    if (accuracy < target)
    {
        count = 0;
    }
    if (accuracy >= target && !ggd->stallUsed)
    {
        count++;
        trophyUpdateMilestone(&ggTrophies[trophyName], count, MILE_ACC);
    }
}

static void drinkWater()
{
    ggd->textOrder = 0;
    readNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_TEXT_PROG], &ggd->textOrder);
    ggd->textOrder++;
    if (ggd->textOrder >= ggGetDrinkTextLen())
    {
        ggd->textOrder = 0;
    }
    if (ggd->textOrder == ggGetDrinkTextLen() - 1)
    {
        trophyUpdate(&ggTrophies[T_TRUTH], 1, true);
    }
    writeNamespaceNvs32(ggNVSSpace[GG_NAMESPACE], ggNVSSpace[GG_TEXT_PROG], ggd->textOrder);
}