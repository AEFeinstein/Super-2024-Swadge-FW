/**
 * @file screensaver.c
 * @author Jeremy Stintzcum
 * @brief Some swadge screensavers. Ruin your battery life
 * @date 2025-05-30
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "screensaver.h"
#include "esp_random.h"
#include "modeIncludeList.h"

//==============================================================================
// Defines
//==============================================================================

#define SUBPIXEL_COUNT 256
#define MAX_VELOCITY   (8 * SUBPIXEL_COUNT)

//==============================================================================
// Consts
//==============================================================================

const char modeName[] = "Screensaver";

/// @brief Contains a sprite for each major mode
static const cnfsFileIdx_t items[] = {
    MAG_FEST_BOUNCER_WSG,
    BARREL_1_WSG,
    MMX_TROPHY_WSG,
};

//==============================================================================
// Function declarations
//==============================================================================

static void screenEnterMode(void);
static void screenExitMode(void);
static void screenMainLoop(int64_t elapsedUs);
static void updateObjects(void);
static void drawScreenSaver(void);

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    vec_t pos;         // Position and bounding box
    vec_t velocity;    // current velocity
    vec_t accumulator; // Velocity added
    wsg_t image;       // Loaded WSG
    bool isActive;     // Should draw
} bouncingObject_t;

typedef struct
{
    bouncingObject_t objs[ARRAY_SIZE(items)];
} screenSaverData_t;

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t screenSaverMode = {
    .modeName          = modeName,
    .wifiMode          = NO_WIFI,
    .overrideUsb       = false,
    .usesAccelerometer = false,
    .overrideSelectBtn = false,
    .fnEnterMode       = screenEnterMode,
    .fnExitMode        = screenExitMode,
    .fnMainLoop        = screenMainLoop,
};

screenSaverData_t* ssd;

//==============================================================================
// Functions
//==============================================================================

static void screenEnterMode(void)
{
    ssd = (screenSaverData_t*)heap_caps_calloc(1, sizeof(screenSaverData_t), MALLOC_CAP_8BIT);
    for (int idx = 0; idx < ARRAY_SIZE(items); idx++)
    {
        loadWsg(items[idx], &ssd->objs[idx].image, true);
        vec_t startPos = {
            .x = (TFT_WIDTH - ssd->objs[idx].image.w) >> 1,
            .y = (TFT_HEIGHT - ssd->objs[idx].image.h) >> 1,
        };
        ssd->objs[idx].pos        = startPos;
        ssd->objs[idx].velocity.x = (esp_random() % MAX_VELOCITY) - (MAX_VELOCITY >> 1);
        ssd->objs[idx].velocity.y = (esp_random() % MAX_VELOCITY) - (MAX_VELOCITY >> 1);
    }
    // Set active images
    ssd->objs[0].isActive = true; // Always active
    if (trophyGetPoints(false, roboRunnerMode.modeName) == 1000)
    {
        ssd->objs[1].isActive = true;
    }
    if (trophyGetPoints(false, mainMenuMode.modeName) == 1000)
    {
        ssd->objs[2].isActive = true;
    }
}

static void screenExitMode(void)
{
    for (int idx = 0; idx < ARRAY_SIZE(items); idx++)
    {
        freeWsg(&ssd->objs[idx].image);
    }
    heap_caps_free(ssd);
}

static void screenMainLoop(int64_t elapsedUs)
{
    // Handle input
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // Allow user to back out of mode
    }

    // Update objects
    updateObjects();

    // Draw
    drawScreenSaver();
}

static void updateObjects()
{
    for (int idx = 0; idx < ARRAY_SIZE(items); idx++)
    {
        bouncingObject_t* bo = &ssd->objs[idx];
        if (!bo->isActive)
        {
            continue;
        }
        // Update position
        bo->accumulator = addVec2d(bo->accumulator, bo->velocity);
        // X
        if (bo->velocity.x < 0)
        {
            // Negative
            while (bo->accumulator.x < -SUBPIXEL_COUNT)
            {
                bo->accumulator.x += SUBPIXEL_COUNT;
                bo->pos.x -= 1;
            }
        }
        else if (bo->velocity.x > 0)
        {
            // Positive
            while (bo->accumulator.x > SUBPIXEL_COUNT)
            {
                bo->accumulator.x -= SUBPIXEL_COUNT;
                bo->pos.x += 1;
            }
        }
        if (bo->pos.x < 0)
        {
            bo->pos.x = 0;
            bo->velocity.x *= -1;
        }
        if (bo->pos.x > TFT_WIDTH - bo->image.w * 2)
        {
            bo->pos.x = TFT_WIDTH - bo->image.w * 2;
            bo->velocity.x *= -1;
        }

        // Y
        if (bo->velocity.y < 0)
        {
            // Negative
            while (bo->accumulator.y < -SUBPIXEL_COUNT)
            {
                bo->accumulator.y += SUBPIXEL_COUNT;
                bo->pos.y -= 1;
            }
        }
        else if (bo->velocity.y > 0)
        {
            // Positive
            while (bo->accumulator.y > SUBPIXEL_COUNT)
            {
                bo->accumulator.y -= SUBPIXEL_COUNT;
                bo->pos.y += 1;
            }
        }
        if (bo->pos.y < 0)
        {
            bo->pos.y = 0;
            bo->velocity.y *= -1;
        }
        if (bo->pos.y > TFT_HEIGHT - bo->image.h * 2)
        {
            bo->pos.y = TFT_HEIGHT - bo->image.h * 2;
            bo->velocity.y *= -1;
        }
    }
}

static void drawScreenSaver()
{
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c000);
    for (int idx = 0; idx < ARRAY_SIZE(items); idx++)
    {
        if (ssd->objs[idx].isActive)
        {
            drawWsgSimpleScaled(&ssd->objs[idx].image, ssd->objs[idx].pos.x, ssd->objs[idx].pos.y, 2, 2);
        }
    }
}