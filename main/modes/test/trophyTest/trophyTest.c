/**
 * @file trophyTest.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @author GEMESIS
 * @brief Tests the functionality of the trophies.
 * @version 0.1
 * @date 2025-01-15
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "trophyTest.h"

//==============================================================================
// Consts
//==============================================================================

static const char trophyModeName[] = "Trophy Test - Long name, short results!";

// Trophy Data example
const trophyData_t testTrophies[] = {
    {
        .title       = "Trigger Trophy",
        .description = "You pressed A!",
        .imageString = "kid0.wsg",
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, set to one
    },
    {
        .title       = "Additive Trophy - Testing",
        .description = "Pressed B ten times!",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 10,
    },
    {
        .title       = "Progress Tropy",
        .description = "Hold down the up button for eight seconds",
        .imageString = "",
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 8,
    },
    {
        .title       = "Checklist",
        .description = "This is gonna need a bunch of verification",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_CHECKLIST,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 0x0007, // Three tasks, 0x01, 0x02, and 0x04
    },
    {
        .title       = "Placeholder",
        .description = "If our eyes aren't real, why can't bees fly?",
        .imageString = "",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 12000000,
    },
    {
        .title       = "Placeholder 2",
        .description = "",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1024,
    },
};

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int idx;
    int bPresses;
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

//==============================================================================
// Variables
//==============================================================================

trophySettings_t tSettings
    = {.animated = false, .drawFromBottom = false, .silent = false, .drawMaxDuration = 8, .slideMaxDuration = 3};

trophyDataList_t trophyTestData = {.settings = &tSettings, .list = testTrophies};

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
    tt           = heap_caps_calloc(sizeof(trophyTest_t), 1, MALLOC_CAP_8BIT);
    tt->idx      = 0;
    tt->bPresses = 0;
}

static void exitTrophy()
{
    heap_caps_free(tt);
}

static void runTrophy(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button == PB_A)
            {
                trophyUpdate(testTrophies[0], 1, true);
            }
            else if (evt.button == PB_B)
            {
                tt->bPresses++;
                trophyUpdate(testTrophies[1], tt->bPresses, true);
            }
            else if (evt.button == PB_UP)
            {
                // TODO: Check how long held down
            }
            else if (evt.button == PB_DOWN)
            {
                // TODO: Task 0x01
            }
            else if (evt.button == PB_RIGHT)
            {
                // TODO: Task 0x02
            }
            else if (evt.button == PB_LEFT)
            {
                // TODO: Task 0x04
            }
            else if (evt.button == PB_START)
            {
                for (int idx = 0; idx < ARRAY_SIZE(testTrophies); idx++)
                {
                    trophyClear(testTrophies[idx]);
                }
                tt->bPresses = 0;
            }
        }
    }
    // Draw instructions
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);
    drawText(getSysFont(), c555, "Press A once", 32, 32);
    drawText(getSysFont(), c555, "Press B ten times", 32, 64);
    drawText(getSysFont(), c555, "Press up for eight seconds", 32, 96);
    int startX = 32;
    int startY = 128;
    drawTextWordWrap(getSysFont(), c555, "Press left, right and down at least once each", &startX, &startY,
                     TFT_WIDTH - 32, TFT_HEIGHT);
    drawText(getSysFont(), c555, "Press pause to reset", 32, 168);
}