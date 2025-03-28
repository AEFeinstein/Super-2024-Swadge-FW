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
        .title       = "Overlong Trophy Name",
        .description = "Description 1",
        .imageString = "kid0.wsg",
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1, // For trigger type, no value is required
    },
    {
        .title       = "Title 2",
        .description = "Description 2",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1024,
    },
    {
        .title       = "Title 3",
        .description = "Description 3",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1024,
    },
    {
        .title       = "Title 4",
        .description = "Description 4",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1024,
    },
    {
        .title       = "Title 5",
        .description = "Description 5",
        .imageString = "kid1.wsg",
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1024,
    },
    {
        .title       = "Title 6",
        .description = "Description 6",
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
    = {.animated = true, .drawFromBottom = true, .silent = false, .drawMaxDuration = 5, .slideMaxDuration = 5};

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
    tt = heap_caps_calloc(sizeof(trophyTest_t), 1, MALLOC_CAP_8BIT);
    tt->idx = 0;
}

static void exitTrophy()
{
    heap_caps_free(tt);
}

static void runTrophy(int64_t elapsedUs)
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if (evt.button & PB_A)
            {  
                trophyUpdate(testTrophies[tt->idx++], 1, true);
                if (tt->idx >= ARRAY_SIZE(testTrophies))
                {
                    tt->idx = 0;
                }
            }
            else if (evt.button & PB_B)
            {
                for (int idx = 0; idx < ARRAY_SIZE(testTrophies); idx++)
                {
                    trophyClear(testTrophies[idx]);
                }
            }
        }   
    }

    drawText(getSysFont(), c555, "Press A to trigger", 32, 32);
}