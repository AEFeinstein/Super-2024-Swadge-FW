/**
 * @file bouncy.c
 * @author Jeremy Stintzcum
 * @brief Toss items around the screen like a DVD screensaver, but not because this isn't a screen saver. Ruin your battery life
 * @date 2025-05-30
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "bouncy.h"
#include "esp_random.h"
#include "modeIncludeList.h"
#include "dance_SmoothRainbow.h"

//==============================================================================
// Defines
//==============================================================================

#define SUBPIXEL_COUNT 256
#define MAX_VELOCITY   (8 * SUBPIXEL_COUNT)

//==============================================================================
// Consts
//==============================================================================

const char modeName[] = "bouncy";

/// @brief Contains a sprite for each major mode
static const cnfsFileIdx_t items[] = {
    MAG_FEST_BOUNCER_WSG,
    BARREL_1_WSG,
    MMX_TROPHY_WSG,
};

static const char* const warningLabels[] = {
    "WARNING!",
    "This mode draws a lot of power! If you'd like to leave the swadge on, consider going into LED dance, which has "
    "been optimized for long term swadge use.",
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    LED_NORTH,
    LED_WEST,
    LED_SOUTH,
    LED_EAST,
} LEDDirections_t;

//==============================================================================
// Function declarations
//==============================================================================

static void screenEnterMode(void);
static void screenExitMode(void);
static void screenMainLoop(int64_t elapsedUs);
static void updateObjects(void);
static void lightLEDs(LEDDirections_t dir);
static void fadeLEDs(void);
static void drawBouncy(void);
static void drawWarning(void);

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
    led_t leds[CONFIG_NUM_LEDS];
    bool explosion;
    int64_t explosionTimer;
    bool displayWarning;
    wsg_t batteryImage;
} bouncyData_t;

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t bouncyMode = {
    .modeName          = modeName,
    .wifiMode          = NO_WIFI,
    .overrideUsb       = false,
    .usesAccelerometer = false,
    .overrideSelectBtn = false,
    .fnEnterMode       = screenEnterMode,
    .fnExitMode        = screenExitMode,
    .fnMainLoop        = screenMainLoop,
};

bouncyData_t* ssd;

//==============================================================================
// Functions
//==============================================================================

static void screenEnterMode(void)
{
    // Set display Hz
    setFrameRateUs(33333); // ~30Hz

    ssd = (bouncyData_t*)heap_caps_calloc(1, sizeof(bouncyData_t), MALLOC_CAP_8BIT);
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
    loadWsg(BATT_1_WSG, &ssd->batteryImage, true);

    // Setup LEDs
    for (int idx = 0; idx < CONFIG_NUM_LEDS; idx++)
    {
        led_t l        = {0};
        ssd->leds[idx] = l;
    }

    // Turn off DAC so no buzzing
    setDacShutdown(true);

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

    ssd->displayWarning = true;
}

static void screenExitMode(void)
{
    freeWsg(&ssd->batteryImage);
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
        // Allow user to back out of mode via holding down "menu"
        // and move past the warning screen
        if (evt.down)
        {
            ssd->displayWarning = false;
        }
    }

    // Update objects
    updateObjects();

    // Color explosion
    if (ssd->explosion)
    {
        ssd->explosionTimer += elapsedUs;
        if (ssd->explosionTimer > 5000000)
        {
            ssd->explosion      = false;
            ssd->explosionTimer = 0;
        }
        danceSmoothRainbow(elapsedUs, 4000, false);
    }
    else
    {
        fadeLEDs();
    }

    // Draw
    if (!ssd->displayWarning)
    {
        drawBouncy();
    }
    else
    {
        drawWarning();
    }
}

static void updateObjects()
{
    for (int idx = 0; idx < ARRAY_SIZE(items); idx++)
    {
        bool hitX            = false;
        bool hitY            = false;
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
            lightLEDs(LED_WEST);
            hitX = true;
        }
        if (bo->pos.x > TFT_WIDTH - bo->image.w * 2)
        {
            bo->pos.x = TFT_WIDTH - bo->image.w * 2;
            bo->velocity.x *= -1;
            lightLEDs(LED_EAST);
            hitX = true;
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
            lightLEDs(LED_NORTH);
            hitY = true;
        }
        if (bo->pos.y > TFT_HEIGHT - bo->image.h * 2)
        {
            bo->pos.y = TFT_HEIGHT - bo->image.h * 2;
            bo->velocity.y *= -1;
            lightLEDs(LED_SOUTH);
            hitY = true;
        }
        if (hitX & hitY)
        {
            ssd->explosion      = true;
            ssd->explosionTimer = 0;
        }
    }
}

static void lightLEDs(LEDDirections_t dir)
{
    led_t randColors = {
        .r = esp_random() % 256,
        .g = esp_random() % 256,
        .b = esp_random() % 256,
    };
    switch (dir)
    {
        case LED_NORTH:
        {
            ssd->leds[1] = randColors;
            ssd->leds[2] = randColors;
            ssd->leds[3] = randColors;
            break;
        }
        case LED_SOUTH:
        {
            ssd->leds[0] = randColors;
            ssd->leds[4] = randColors;
            ssd->leds[6] = randColors;
            ssd->leds[7] = randColors;
            break;
        }
        case LED_EAST:
        {
            ssd->leds[0] = randColors;
            ssd->leds[1] = randColors;
            ssd->leds[8] = randColors;
            break;
        }
        case LED_WEST:
        {
            ssd->leds[3] = randColors;
            ssd->leds[5] = randColors;
            ssd->leds[6] = randColors;
            break;
        }
        default:
        {
            break;
        }
    }
}

static void fadeLEDs()
{
    for (int idx = 0; idx < CONFIG_NUM_LEDS; idx++)
    {
        if (ssd->leds[idx].r > 5)
        {
            ssd->leds[idx].r -= 5;
        }
        if (ssd->leds[idx].g > 5)
        {
            ssd->leds[idx].g -= 5;
        }
        if (ssd->leds[idx].b > 5)
        {
            ssd->leds[idx].b -= 5;
        }
    }
    setLeds(ssd->leds, CONFIG_NUM_LEDS);
}

static void drawBouncy()
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

static void drawWarning()
{
    drawText(getSysFont(), c500, warningLabels[0], (TFT_WIDTH - textWidth(getSysFont(), warningLabels[0])) >> 1, 60);
    int16_t xCoord = 8;
    int16_t yCoord = 120;
    drawTextWordWrap(getSysFont(), c555, warningLabels[1], &xCoord, &yCoord, TFT_WIDTH - 8, TFT_HEIGHT);
    drawWsgSimple(&ssd->batteryImage, (TFT_WIDTH - ssd->batteryImage.w) >> 1, 200);
}