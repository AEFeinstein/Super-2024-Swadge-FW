/**
 * @file trophyCase.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Displays all the trophies
 * @date 2025-05-14
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "trophyCase.h"
#include "modeIncludeList.h"
#include "menu.h"

#define SCROLL_SPEED 32

// Drawing magic numbers
#define X_START          32
#define Y_START          8
#define LINE_SPACING     24
#define INDIV_SCORE_LINE 4

//==============================================================================
// Consts
//==============================================================================

const char tCaseModeName[]           = "Trophy Case";
static const char* const menuItems[] = {"Scores & Latest", "Mode: "};
static const char individualStr[]    = "Individual modes";
static const char latestStr[]        = "Latest Trophy:";
static const char noTrophiesYet[]    = "You haven't won any trophies yet!";
static const char exitStr[]          = "Exit Case";

static const char* const caseOptions[] = {"All", "Unlocked", "Locked"};
static const int32_t caseSettings[]    = {TROPHY_DISPLAY_ALL, TROPHY_DISPLAY_UNLOCKED, TROPHY_DISPLAY_LOCKED};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    TC_MENU,
    TC_STATS,
    TC_DISPLAY,
} tCaseStateEnum_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Mode specific
    tCaseStateEnum_t state;
    int idx;
    trophyListDisplayMode_t dm;

    // Menu items
    menu_t* menu;
    menuMegaRenderer_t* rnd;
} tCase_t;

//==============================================================================
// Function Declarations
//==============================================================================

/**
 * @brief Entrance into the mode
 *
 */
static void enterTCase(void);

/**
 * @brief Mode cleanup
 *
 */
static void exitTCase(void);

/**
 * @brief Main Loop
 *
 * @param elapsedUs Amount of time since last called
 */
static void runTCase(int64_t elapsedUs);

/**
 * @brief Menu callback function
 *
 * @param label
 * @param selected
 * @param settingVal
 */
static bool tCaseMenuCb(const char* label, bool selected, uint32_t settingVal);

/**
 * @brief DRaw the score and latests trophy screen
 *
 * @param fnt Font to use
 * @param yOffset How far to draw the text off center.
 */
static void tCaseDrawStats(font_t* fnt, int yOffset);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t tCaseMode = {
    .modeName          = tCaseModeName,
    .wifiMode          = NO_WIFI,
    .overrideUsb       = false,
    .usesAccelerometer = false,
    .usesThermometer   = false,
    .overrideSelectBtn = false,
    .fnEnterMode       = enterTCase,
    .fnExitMode        = exitTCase,
    .fnMainLoop        = runTCase,
};

static tCase_t* tc;

//==============================================================================
// Function
//==============================================================================

static void enterTCase(void)
{
    tc       = heap_caps_calloc(sizeof(tCase_t), 1, MALLOC_CAP_8BIT);
    tc->menu = initMenu(tCaseModeName, tCaseMenuCb);
    tc->rnd  = initMenuMegaRenderer(NULL, NULL, NULL);
    addSingleItemToMenu(tc->menu, menuItems[0]);
    // Note, TROPHY_DISPLAY_INCL_HIDDEN is not shown in this list
    settingParam_t caseOptParams = {
        .def = caseSettings[0],
        .key = NULL,
        .min = caseSettings[0],
        .max = caseSettings[ARRAY_SIZE(caseSettings) - 1],
    };
    addSettingsOptionsItemToMenu(tc->menu, menuItems[1], caseOptions, caseSettings, ARRAY_SIZE(caseOptions),
                                 &caseOptParams, 0);

    // Add all other modes to menu
    for (int idx = 0; idx < modeListGetCount(); idx++)
    {
        if (allSwadgeModes[idx]->trophyData != NULL
            && strcmp(allSwadgeModes[idx]->modeName, trophyTestMode.modeName) != 0)
        {
            addSingleItemToMenu(tc->menu, allSwadgeModes[idx]->modeName);
        }
    }
    addSingleItemToMenu(tc->menu, exitStr);
}

static void exitTCase(void)
{
    deinitMenuMegaRenderer(tc->rnd);
    deinitMenu(tc->menu);
    heap_caps_free(tc);
}

static void runTCase(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch (tc->state)
    {
        case TC_DISPLAY:
        {
            // Predraw because use after free
            trophyDrawList(getSysFont(), tc->idx);
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_UP)
                    {
                        tc->idx -= SCROLL_SPEED;
                    }
                    else if (evt.button & PB_DOWN)
                    {
                        tc->idx += SCROLL_SPEED;
                    }
                    else
                    {
                        tc->state = TC_MENU;
                        trophyDrawListDeinit();
                    }
                }
            }
            break;
        }
        case TC_STATS:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_UP)
                    {
                        tc->idx -= SCROLL_SPEED;
                    }
                    else if (evt.button & PB_DOWN)
                    {
                        tc->idx += SCROLL_SPEED;
                    }
                    else
                    {
                        tc->state = TC_MENU;
                    }
                }
            }
            tCaseDrawStats(getSysFont(), tc->idx);
            break;
        }
        case TC_MENU:
        default:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down && evt.button & PB_B)
                {
                    switchToSwadgeMode(&mainMenuMode);
                }
                tc->menu = menuButton(tc->menu, evt);
            }
            drawMenuMega(tc->menu, tc->rnd, elapsedUs);
            break;
        }
    }
}

static bool tCaseMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        for (int idx = 0; idx < modeListGetCount(); idx++)
        {
            if (label == allSwadgeModes[idx]->modeName)
            {
                trophySetSystemData(allSwadgeModes[idx]->trophyData, allSwadgeModes[idx]->modeName);
                trophyDrawListInit(tc->dm);
                tc->state = TC_DISPLAY;
                tc->idx   = 0;
            }
        }
        if (label == menuItems[0])
        {
            tc->state = TC_STATS;
            tc->idx   = 0;
        }
        else if (label == exitStr)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    if (label == menuItems[1])
    {
        tc->dm = settingVal;
    }

    return false;
}

static void tCaseDrawStats(font_t* fnt, int yOffset)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);

    // Title
    drawText(fnt, c555, menuItems[0], X_START, Y_START - yOffset);
    drawLine(X_START, LINE_SPACING - yOffset, TFT_WIDTH - X_START, LINE_SPACING - yOffset, c555, 0);

    // Total score
    char buffer[64];
    snprintf(buffer, sizeof(buffer) - 1, "Total score: %" PRId16, trophyGetPoints(true, NULL));
    drawText(fnt, c544, buffer, X_START, Y_START + LINE_SPACING - yOffset);

    // Latest mode won
    const trophyData_t* td = trophyGetLatest();
    drawText(fnt, c443, latestStr, X_START, (2 * LINE_SPACING) - yOffset);
    int16_t x = X_START;
    int16_t y = (3 * LINE_SPACING) - (yOffset + Y_START);
    if (td != NULL)
    {
        drawTextWordWrap(fnt, c545, td->title, &x, &y, TFT_WIDTH - X_START, (5 * LINE_SPACING) - yOffset);
    }
    else
    {
        drawTextWordWrap(fnt, c500, noTrophiesYet, &x, &y, TFT_WIDTH - X_START, (5 * LINE_SPACING) - yOffset);
    }

    // Individual mode scores
    drawText(fnt, c555, individualStr, X_START, Y_START + (INDIV_SCORE_LINE * LINE_SPACING) - yOffset);
    drawLine(X_START, (INDIV_SCORE_LINE + 1) * LINE_SPACING - yOffset, TFT_WIDTH - X_START,
             (INDIV_SCORE_LINE + 1) * LINE_SPACING - yOffset, c555, 0);
    int line = INDIV_SCORE_LINE;
    for (int idx = 0; idx < modeListGetCount(); idx++)
    {
        if (allSwadgeModes[idx]->trophyData != NULL
            && strcmp(allSwadgeModes[idx]->modeName, trophyTestMode.modeName) != 0)
        {
            snprintf(buffer, sizeof(buffer) - 1, "%s: %" PRId16, allSwadgeModes[idx]->modeName,
                     trophyGetPoints(false, allSwadgeModes[idx]->trophyData->settings->namespaceKey));
            drawText(fnt, c454, buffer, X_START, (Y_START + LINE_SPACING + (line++) * LINE_SPACING) - yOffset);
        }
    }
}
