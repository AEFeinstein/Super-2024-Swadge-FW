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

static const char trophyModeName[] = "TrophyTest";

// Trophy Data example
const trophyData_t testTrophies[] = {
    {
        .title       = "Title 1",
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
};

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    trophyData_t t[2]; //< Example trophy
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
    = {.animated = true, .drawFromBottom = true, .silent = false, .drawMaxDuration = 500, .slideMaxDuration = 200};

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
    tt = (trophyTest_t*)heap_caps_calloc(1, sizeof(trophyTest_t), MALLOC_CAP_8BIT);
    // Test
    bool gre = true;
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
            if (evt.button & PB_A)
            {  
                trophyUpdate(testTrophies[0], 1, true);
            }
            else if (evt.button & PB_B)
            {
                trophyClear(testTrophies[0]);
            }
        }
        
    }
}