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

//==============================================================================
// Consts
//==============================================================================

const char tCaseModeName[]         = "Trophy Case";
static const char* const menuItems[] = {"Stats"};

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

    // Menu items
    menu_t* menu;
    menuManiaRenderer_t* rnd;
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
static void tCaseMenuCb(const char* label, bool selected, uint32_t settingVal);

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
    tc->rnd  = initMenuManiaRenderer(NULL, NULL, NULL);
    addSingleItemToMenu(tc->menu, menuItems[0]);

    // Add all other modes to menu
    for (int idx = 0; idx < modeListGetCount(); idx++)
    {
        if (allSwadgeModes[idx]->trophyData != NULL)
        {
            addSingleItemToMenu(tc->menu, allSwadgeModes[idx]->modeName);
        }
    }
}

static void exitTCase(void)
{
    deinitMenuManiaRenderer(tc->rnd);
    deinitMenu(tc->menu);
    free(tc);
}

static void runTCase(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch (tc->state)
    {
        case TC_DISPLAY:
        {
            while(checkButtonQueueWrapper(&evt))
            {
            }
            break;
        }
        case TC_STATS:
        {
            while(checkButtonQueueWrapper(&evt))
            {
            }
            break;
        }
        case TC_MENU:
        default:
        {
            while(checkButtonQueueWrapper(&evt))
            {
                tc->menu = menuButton(tc->menu, evt);
            }
            drawMenuMania(tc->menu, tc->rnd, elapsedUs);
            break;
        }
    }
}

static void tCaseMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    
}