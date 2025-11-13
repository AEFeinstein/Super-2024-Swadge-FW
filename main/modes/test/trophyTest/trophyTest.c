/**
 * @file trophyTest.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Tests the functionality of the trophies.
 * @date 2025-01-15
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "trophyTest.h"
#include "menu.h"

//==============================================================================
// Includes
//==============================================================================

#define SECOND_US 1000000

//==============================================================================
// Consts
//==============================================================================

static const char trophyModeName[] = "Trophy Test";

static const char* const textBlobs[] = {
    "Press A once",
    "Press B ten times",
    "Press up for eight seconds",
    "Press left, right and down at least once each",
    "Press pause to go to menu",
    "Trophy Test",
    "Clear progress",
    "Trophy Case",
    "Display mode: ",
    "Go back",
};

static const char* const caseOptions[] = {"All", "Unlocked", "Locked", "Hidden"};
static const int32_t caseSettings[]
    = {TROPHY_DISPLAY_ALL, TROPHY_DISPLAY_UNLOCKED, TROPHY_DISPLAY_LOCKED, TROPHY_DISPLAY_INCL_HIDDEN};

// Trophy Data
const trophyData_t trophyTestModeTrophies[] = {
    {
        .title       = "Trigger Trophy",
        .description = "You pressed A!",
        .image       = KID_0_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "Additive Trophy - Testing a very long trophy na",
        .description = "Pressed B ten times!",
        .image       = KID_1_WSG,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 10,
    },
    {
        .title       = "Progress Trophy",
        .description = "Hold down the up button for eight seconds",
        .image       = NO_IMAGE_SET, // Hardcoded "Ignore" value
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 8,
        .hidden      = true,
        .noImage     = true,
    },
    {
        .title       = "Checklist",
        .description = "This is gonna need a bunch of verification, but like has a very long description",
        .image       = NO_IMAGE_SET, // If set like this, it will draw a default trophy
        .type        = TROPHY_TYPE_CHECKLIST,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 0x0007, // Three tasks, 0x01, 0x02, and 0x04
    },
};

// Individual mode settings
trophySettings_t trophyTestModeTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    TROPHY_TEST_TESTING,
    TROPHY_TEST_MENU,
    TROPHY_TEST_DISPLAYING,
} trophyTestStateEnum_t;

typedef enum
{
    CLT_DOWN,
    CLT_LEFT,
    CLT_RIGHT,
} ChecklistTask_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    // Trophy Vars
    // A presses
    int32_t aPresses;

    // B presses
    int32_t bPresses;

    // Up held duration
    int64_t heldTimer;
    int32_t upTime; // HeldTimer / SECOND_US
    bool timing;

    // Checklist
    int32_t checklistFlags;

    // Trophy Case
    int idx; ///< Current display index
    trophyListDisplayMode_t dm;

    // Menu
    trophyTestStateEnum_t state;
    menu_t* menu;
    menuMegaRenderer_t* rndr;
} trophyTest_t;

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Entrance into the mode
 *
 */
static void enterTrophy(void);

/**
 * @brief Mode cleanup
 *
 */
static void exitTrophy(void);

/**
 * @brief Main Loop
 *
 * @param elapsedUs Amount of time since last called
 */
static void runTrophy(int64_t elapsedUs);

/**
 * @brief Callback for options menu
 *
 * @param label Used to locate option
 * @param selected If option was selected or just highlighted
 * @param settingVal Value when a setting is changed
 */
static void trophyMenuCb(const char* label, bool selected, uint32_t settingVal);

//==============================================================================
// Variables
//==============================================================================

trophyDataList_t trophyTestData = {.settings = &trophyTestModeTrophySettings,
                                   .list     = trophyTestModeTrophies,
                                   .length   = ARRAY_SIZE(trophyTestModeTrophies)};

swadgeMode_t trophyTestMode = {.modeName                 = trophyModeName,
                               .wifiMode                 = NO_WIFI,
                               .overrideUsb              = false,
                               .usesAccelerometer        = false,
                               .usesThermometer          = false,
                               .overrideSelectBtn        = false,
                               .fnEnterMode              = enterTrophy,
                               .fnExitMode               = exitTrophy,
                               .fnMainLoop               = runTrophy,
                               .fnAudioCallback          = NULL,
                               .fnBackgroundDrawCallback = NULL,
                               .fnEspNowRecvCb           = NULL,
                               .fnEspNowSendCb           = NULL,
                               .fnAdvancedUSB            = NULL,
                               .trophyData               = &trophyTestData};

static trophyTest_t* tt;

//==============================================================================
// Functions
//==============================================================================

static void enterTrophy()
{
    // Allocate memory
    tt = heap_caps_calloc(sizeof(trophyTest_t), 1, MALLOC_CAP_8BIT);

    // Initialize vars from disk
    tt->aPresses       = trophyGetSavedValue(trophyTestModeTrophies[0]);
    tt->bPresses       = trophyGetSavedValue(trophyTestModeTrophies[1]);
    tt->upTime         = trophyGetSavedValue(trophyTestModeTrophies[2]);
    tt->checklistFlags = trophyGetSavedValue(trophyTestModeTrophies[3]);

    tt->heldTimer = 0;

    // Menu
    tt->menu = initMenu(textBlobs[5], trophyMenuCb);
    tt->rndr = initMenuMegaRenderer(NULL, NULL, NULL);
    addSingleItemToMenu(tt->menu, textBlobs[6]);
    addSingleItemToMenu(tt->menu, textBlobs[7]);
    addSettingsOptionsItemToMenu(tt->menu, textBlobs[8], caseOptions, caseSettings, ARRAY_SIZE(caseOptions),
                                 getScreensaverTimeSettingBounds(), 0);
    addSingleItemToMenu(tt->menu, textBlobs[9]);

    tt->state = TROPHY_TEST_TESTING;
}

static void exitTrophy()
{
    deinitMenuMegaRenderer(tt->rndr);
    deinitMenu(tt->menu);
    heap_caps_free(tt);
}

static void runTrophy(int64_t elapsedUs)
{
    buttonEvt_t evt;
    switch (tt->state)
    {
        case TROPHY_TEST_MENU:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                tt->menu = menuButton(tt->menu, evt);
            }
            drawMenuMega(tt->menu, tt->rndr, elapsedUs);
            break;
        }
        case TROPHY_TEST_DISPLAYING:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button & PB_UP)
                    {
                        tt->idx--;
                        if (tt->idx < -1)
                        {
                            tt->idx = 14;
                        }
                    }
                    else if (evt.button & PB_DOWN)
                    {
                        tt->idx++;
                        if (tt->idx >= 14)
                        {
                            tt->idx = -1;
                        }
                    }
                    else
                    {
                        tt->state = TROPHY_TEST_MENU;
                        trophyDrawListDeinit();
                        return;
                    }
                }
            }
            trophyDrawList(getSysFont(), tt->idx * 20);
            break;
        }
        default:
        {
            while (checkButtonQueueWrapper(&evt))
            {
                if (evt.down)
                {
                    if (evt.button == PB_A)
                    {
                        tt->aPresses++;
                        trophyUpdate(trophyTestModeTrophies[0], 1, true);
                    }
                    else if (evt.button == PB_B)
                    {
                        tt->bPresses++;
                        trophyUpdateMilestone(trophyTestModeTrophies[1], tt->bPresses, 20);
                    }
                    else if (evt.button == PB_UP)
                    {
                        // Check how long held down
                        tt->timing    = true;
                        tt->heldTimer = 0; // Reset each time;
                    }
                    else if (evt.button == PB_DOWN)
                    {
                        if (checkBitFlag(tt->checklistFlags, CLT_DOWN))
                        {
                            setBitFlag(&tt->checklistFlags, CLT_DOWN, false);
                            trophySetChecklistTask(trophyTestModeTrophies[3], CLT_DOWN, false, true);
                        }
                        else
                        {
                            setBitFlag(&tt->checklistFlags, CLT_DOWN, true);
                            trophySetChecklistTask(trophyTestModeTrophies[3], CLT_DOWN, true, true);
                        }
                    }
                    else if (evt.button == PB_LEFT)
                    {
                        if (checkBitFlag(tt->checklistFlags, CLT_LEFT))
                        {
                            setBitFlag(&tt->checklistFlags, CLT_LEFT, false);
                            trophySetChecklistTask(trophyTestModeTrophies[3], CLT_LEFT, false, true);
                        }
                        else
                        {
                            setBitFlag(&tt->checklistFlags, CLT_LEFT, true);
                            trophySetChecklistTask(trophyTestModeTrophies[3], CLT_LEFT, true, true);
                        }
                    }
                    else if (evt.button == PB_RIGHT)
                    {
                        if (checkBitFlag(tt->checklistFlags, CLT_RIGHT))
                        {
                            setBitFlag(&tt->checklistFlags, CLT_RIGHT, false);
                            trophySetChecklistTask(trophyTestModeTrophies[3], CLT_RIGHT, false, true);
                        }
                        else
                        {
                            setBitFlag(&tt->checklistFlags, CLT_RIGHT, true);
                            trophySetChecklistTask(trophyTestModeTrophies[3], CLT_RIGHT, true, true);
                        }
                    }
                    else if (evt.button == PB_START)
                    {
                        tt->state = TROPHY_TEST_MENU;
                    }
                }
                else if (evt.button & PB_UP)
                {
                    // Stop count
                    tt->timing = false;
                    // Save to NVS
                    if (tt->upTime < (tt->heldTimer / SECOND_US))
                    {
                        tt->upTime = (tt->heldTimer / SECOND_US);
                    }
                    trophyUpdate(trophyTestModeTrophies[2], tt->upTime, true);
                }
            }
            // Draw instructions
            fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);

            char buffer[64];

            // Trigger
            drawText(getSysFont(), c555, textBlobs[0], 32, 16);
            snprintf(buffer, sizeof(buffer) - 1, "Current: %" PRId32, tt->aPresses);
            drawText(getSysFont(), c500, buffer, 32, 32);

            // Additive
            drawText(getSysFont(), c555, textBlobs[1], 32, 48);
            snprintf(buffer, sizeof(buffer) - 1, "Current: %" PRId32, tt->bPresses);
            drawText(getSysFont(), c050, buffer, 32, 64);

            // Progress
            drawText(getSysFont(), c555, textBlobs[2], 32, 80);
            snprintf(buffer, sizeof(buffer) - 1, "Previous Best: %" PRId32, tt->upTime);
            drawText(getSysFont(), c005, buffer, 32, 96);
            snprintf(buffer, sizeof(buffer) - 1, "Current: %f", (tt->heldTimer * 1.0f) / SECOND_US);
            drawText(getSysFont(), c005, buffer, 32, 112);

            // Checklist
            int16_t startX = 32;
            int16_t startY = 130;
            drawTextWordWrap(getSysFont(), c555, textBlobs[3], &startX, &startY, TFT_WIDTH - 32, TFT_HEIGHT);
            paletteColor_t c = checkBitFlag(tt->checklistFlags, CLT_DOWN) ? c550 : c333;
            drawText(getSysFont(), c, "Down", 70, 154);
            c = checkBitFlag(tt->checklistFlags, CLT_LEFT) ? c505 : c333;
            drawText(getSysFont(), c, "Left", 32, 154);
            c = checkBitFlag(tt->checklistFlags, CLT_RIGHT) ? c055 : c333;
            drawText(getSysFont(), c, "Right", 108, 154);

            // Get latest trophy
            trophyData_t td = trophyGetLatest();
            snprintf(buffer, sizeof(buffer) - 1, "Latest trophy: %s", td.title);
            drawText(getSysFont(), c555, buffer, 32, 190);

            // Draw points
            int modeTotal = trophyGetPoints(false, NULL);
            int total     = trophyGetPoints(true, NULL);
            snprintf(buffer, sizeof(buffer) - 1, "Mode Total: %d, total: %d", modeTotal, total);
            drawText(getSysFont(), c555, buffer, 32, 206);

            // Go to menu
            drawText(getSysFont(), c555, textBlobs[4], 32, TFT_HEIGHT - 16);
            break;
        }
    }

    // Timer
    if (tt->timing)
    {
        tt->heldTimer += elapsedUs;
    }
}

static void trophyMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == textBlobs[6])
        {
            for (int idx = 0; idx < ARRAY_SIZE(trophyTestModeTrophies); idx++)
            {
                trophyClear(trophyTestModeTrophies[idx]);
            }
            tt->aPresses       = 0;
            tt->bPresses       = 0;
            tt->upTime         = 0;
            tt->heldTimer      = 0;
            tt->checklistFlags = 0;
        }
        else if (label == textBlobs[7])
        {
            tt->state = TROPHY_TEST_DISPLAYING;
            trophyDrawListInit(tt->dm);
            trophyDrawListColors(c000, c111, c222, c333, c555, c500);
        }
        else
        {
            tt->state = TROPHY_TEST_TESTING;
        }
    }
    if (label == textBlobs[8])
    {
        tt->dm = settingVal;
    }
}