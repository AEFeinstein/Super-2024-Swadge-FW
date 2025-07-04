/**
 * @file jerkChicken.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A mode about jerking the chicken. No, not like the food. Not that way either.
 * @version 0.1
 * @date 2025-06-03
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "jerkChicken.h"

//==============================================================================
// Defines
//==============================================================================

#define JERK_VALUE 1000

//==============================================================================
// Consts
//==============================================================================

const char chickenModeName[] = "Jerk Chicken";

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int16_t xComp, yComp;
    bool jerked;
} chickenData_t;

//==============================================================================
// Function Declarations
//==============================================================================

// SwadgeMode functions
static void enterChicken(void);
static void exitChicken(void);
static void chickenLoop(int64_t elapsedUs);

// Draw routines
static void drawChicken(int64_t elapsedUs);

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t chickenMode = {
    .modeName          = chickenModeName,
    .fnEnterMode       = enterChicken,
    .fnExitMode        = exitChicken,
    .fnMainLoop        = chickenLoop,
    .wifiMode          = NO_WIFI,
    .usesAccelerometer = true,
};

chickenData_t* cd;

//==============================================================================
// Functions
//==============================================================================

static void enterChicken(void)
{
    cd = (chickenData_t*)heap_caps_calloc(1, sizeof(chickenData_t), MALLOC_CAP_8BIT);
}

static void exitChicken(void)
{
    heap_caps_free(cd);
}

static void chickenLoop(int64_t elapsedUs)
{
    // Input
    accelIntegrate();
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // TODO
    }
    int16_t prevX = cd->xComp;
    int16_t prevY = cd->yComp;
    if (ESP_OK == accelGetSteeringAngleDegrees(&cd->xComp, &cd->yComp))
    {
        if (prevX > cd->xComp + JERK_VALUE)
        {
            cd->jerked = true;
        }
    }

    // Draw
    drawChicken(elapsedUs);
}

static void drawChicken(int64_t elapsedUs)
{
    clearPxTft();
    drawCircleFilledQuadrants(TFT_WIDTH >> 1, TFT_HEIGHT >> 1, 64, true, false, true, false, c004);
    drawCircleFilledQuadrants(TFT_WIDTH >> 1, TFT_HEIGHT >> 1, 64, false, true, false, true, c400);
    char buffer[64];
    snprintf(buffer, sizeof(buffer) - 1, "X: %d, Y: %d aTan: %f" PRId16 PRId16, cd->xComp, cd->yComp,
             atan2(cd->xComp, cd->yComp));
    drawText(getSysFont(), c555, buffer, 0, 210);
    drawLineFast(TFT_WIDTH >> 1, TFT_HEIGHT >> 1, (TFT_WIDTH >> 1) - (cd->xComp >> 6),
                 (TFT_HEIGHT >> 1) + (cd->yComp >> 6), c050);
    if (cd->jerked)
    {
        cd->jerked = false;
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c050);
    }
}