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
        .maxVal      = 1,
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

trophyDataList_t trophyTestData = {.settings = &tSettings};

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

    // Load a specific set of values to check if it works.
    /* strcpy(tt->t[0].title, trophyNames[0]);
    strcpy(tt->t[0].imageString, trophyWSGs[0]);
    strcpy(tt->t[0].description, trophyDescs[0]);
    tt->t[0].currentValue = 10;
    tt->t[0].maxValue     = 25;
    tt->t[0].points       = 69;
    tt->t[0].type         = TROPHY_TYPE_ADDITIVE;
    loadImage(0, tt->t[0].imageString);

    strcpy(tt->t[1].title, trophyNames[1]);
    strcpy(tt->t[1].imageString, "");
    strcpy(tt->t[1].description, trophyDescs[1]);
    tt->t[1].currentValue = 25;
    tt->t[1].maxValue     = 25;
    tt->t[1].points       = 69;
    tt->t[1].type         = TROPHY_TYPE_ADDITIVE;
    loadImage(1, tt->t[1].imageString); */

    trophySetValues(&tt->t[0], &trophyTestData, 0);
    trophySetValues(&tt->t[1], &trophyTestData, 1);
}

static void exitTrophy()
{
    // Test
    unloadImage(0);
    unloadImage(1);

    heap_caps_free(tt);
}

static void runTrophy(int64_t elapsedUs)
{
    // Test
    trophyDrawDataDirectly(tt->t[0], 0, getSysFont());
    trophyDrawDataDirectly(tt->t[1], 50, getSysFont());
}