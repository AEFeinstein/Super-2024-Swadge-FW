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

//==============================================================================
// Consts
//==============================================================================

const char tCaseModeName[]           = "Trophy Case";
static const char* const menuItems[] = {"Scores", "Mode: "};
static const char scoreStr[]         = "Scores";
static const char individualStr[]    = "Individual modes";
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
    addSettingsOptionsItemToMenu(tc->menu, menuItems[1], caseOptions, caseSettings, ARRAY_SIZE(caseOptions),
                                 getScreensaverTimeSettingBounds(), 0);

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
    drawText(fnt, c555, scoreStr, 32, 8 - yOffset);
    char buffer[64];
    snprintf(buffer, sizeof(buffer) - 1, "Total score: %" PRId16, trophyGetPoints(true, NULL));
    drawText(fnt, c544, buffer, 96, 8 - yOffset);
    drawLine(32, 24 - yOffset, TFT_WIDTH - 32, 24 - yOffset, c555, 0);
    drawText(fnt, c454, individualStr, 32, 32 - yOffset);
    int line = 1;
    for (int idx = 0; idx < modeListGetCount(); idx++)
    {
        if (allSwadgeModes[idx]->trophyData != NULL
            && strcmp(allSwadgeModes[idx]->modeName, trophyTestMode.modeName) != 0)
        {
            snprintf(buffer, sizeof(buffer) - 1, "%s: %" PRId16, allSwadgeModes[idx]->modeName,
                     trophyGetPoints(false, allSwadgeModes[idx]->trophyData->settings->namespaceKey));
            drawText(fnt, c454, buffer, 32, (32 + (line++) * 24) - yOffset);
        }
    }
}
