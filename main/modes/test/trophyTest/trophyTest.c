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
#include "trophy.h"

//==============================================================================
// Consts
//==============================================================================

static const char trophyModeName[] = "TrophyTest";

static const char* const trophyNames[]
    = {"No! He's just a kid!", "Test1", "Test2", "Test3", "Test4", "Test5", "Test6", "Test7", "Test8", "Test9"};
static const char* const trophyDescs[]
    = {"Eat ten doughnutbois. You freaking monster.", "Test1 Description", "Test2 Description", "Test3 Description", "Test4 Description",
       "Test5 Description", "Test6 Description", "Test7 Description", "Test8 Description", "Test9 Description"};
static const char* const trophyWSGs[] = {"kid0.wsg", "Test1.wsg", "Test2.wsg", "Test3.wsg", "Test4.wsg",
                                         "Test5.wsg", "Test6.wsg", "Test7.wsg", "Test8.wsg", "Test9.wsg"};

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    trophy_t t; //< Example trophy
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

swadgeMode_t trophyTestMode = {
    .modeName                 = trophyModeName,
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
};

static trophyTest_t* tt;

//==============================================================================
// Functions
//==============================================================================

static void enterTrophy()
{
    tt = (trophyTest_t*)heap_caps_calloc(1, sizeof(trophyTest_t), MALLOC_CAP_8BIT);

    // NOTE: Only called here because there's no default font.
    trophySystemSetFont("ibm_vga8.font");

    // Example code
    // Values: From Bottom, durations in tenths of a second, if it should animate, animation time in tenths of a second
    trophySystemInit(false, 500, false, 5);

    //trophyInit(trophyModeName, trophyNames[0], trophyWSGs[0], trophyDescs[0], TROPHY_TYPE_ADDITIVE, 10, 20);

    // Load a specific set of values to check if it works.
    strcpy(tt->t.title, trophyNames[0]);
    strcpy(tt->t.imageString, trophyWSGs[0]);
    strcpy(tt->t.description, trophyDescs[0]);
    tt->t.currentValue = 10;
    tt->t.maxValue = 25;
    tt->t.points = 69;
    tt->t.type = TROPHY_TYPE_ADDITIVE;
    loadImage(0, tt->t.imageString);
    loadImage(1, tt->t.imageString);
}

static void exitTrophy()
{
    // Test
    unloadImage(0);
    unloadImage(1);

    // NOTE: DeInit font. Only used because no default font in swadge
    trophySystemClearFont();

    heap_caps_free(tt);
}

static void runTrophy(int64_t elapsedUs)
{
    trophyDrawDataDirectly(tt->t, 0);
}