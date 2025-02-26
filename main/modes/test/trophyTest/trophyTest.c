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
static const char* const trophyNames[] = {"Locked Trophy", "Unlocked Trophy"};
static const char* const trophyDescs[] = {
    "This one shows a locked trophy.",
    "This one is unlocked!",
};
static const char* const trophyWSGs[]        = {"kid0.wsg", "Test1.wsg"};
static const trophyTypes_t trophyTypes[]     = {TROPHY_TYPE_TRIGGER, TROPHY_TYPE_ADDITIVE};
static const trophyDifficulty_t trophyDiff[] = {TROPHY_DIFF_EASY, TROPHY_DIFF_HARD};
static const int32_t trophyMaxVals[]         = {1, 32};

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    trophy_t t[2]; //< Example trophy
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

static void initTrophy(void);

//==============================================================================
// Variables
//==============================================================================

trophySettings_t tSettings
    = {.animated = true, .drawFromBottom = true, .silent = false, .drawMaxDuration = 500, .slideMaxDuration = 200};

trophyData_t trophyTestData
    = {.length = 2, .names = trophyNames, .descriptions = trophyDescs, .wsgs = trophyWSGs, 
        .types = trophyTypes, .difficulty = trophyDiff, .maxVals = trophyMaxVals};

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
                               .tSettings                = &tSettings};

static trophyTest_t* tt;

//==============================================================================
// Functions
//==============================================================================

static void enterTrophy()
{
    tt = (trophyTest_t*)heap_caps_calloc(1, sizeof(trophyTest_t), MALLOC_CAP_8BIT);

    // Load a specific set of values to check if it works.
    strcpy(tt->t[0].title, trophyNames[0]);
    strcpy(tt->t[0].imageString, trophyWSGs[0]);
    strcpy(tt->t[0].description, trophyDescs[0]);
    tt->t[0].currentValue = 10;
    tt->t[0].maxValue     = 25;
    tt->t[0].points       = 69;
    tt->t[0].type         = TROPHY_TYPE_ADDITIVE;
    loadImage(0, tt->t[0].imageString);

    strcpy(tt->t[1].title, trophyNames[1]);
    strcpy(tt->t[1].imageString, trophyWSGs[1]);
    strcpy(tt->t[1].description, trophyDescs[1]);
    tt->t[1].currentValue = 25;
    tt->t[1].maxValue     = 25;
    tt->t[1].points       = 69;
    tt->t[1].type         = TROPHY_TYPE_ADDITIVE;
    loadImage(1, tt->t[1].imageString);
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