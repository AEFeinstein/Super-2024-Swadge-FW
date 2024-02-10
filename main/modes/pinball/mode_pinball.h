#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <esp_random.h>
#include "swadge2024.h"
#include "fp_math.h"

//==============================================================================
// Defines
//==============================================================================

#define PIN_US_PER_FRAME 16667
#define NUM_ZONES        32

#define MAX_NUM_WALLS 100
#define MAX_NUM_BALLS 10

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    vec_q24_8 pos;
    vec_q24_8 vel; // Velocity is in pixels per frame (@ 60fps, so pixels per 16.7ms)
    q24_8 radius;
    uint32_t zoneMask;
    paletteColor_t color;
    bool filled;
} pbCircle_t;

typedef struct
{
    vec_q24_8 p1;
    vec_q24_8 p2;
    uint32_t zoneMask;
    paletteColor_t color;
} pbLine_t;

typedef struct
{
    vec_q24_8 pos; ///< The position the top left corner of the rectangle
    q24_8 width;   ///< The width of the rectangle
    q24_8 height;  ///< The height of the rectangle
    uint32_t zoneMask;
    paletteColor_t color;
} pbRect_t;

typedef struct
{
    pbCircle_t balls[MAX_NUM_BALLS];
    uint32_t numBalls;
    pbLine_t walls[MAX_NUM_WALLS];
    uint32_t numWalls;
    pbCircle_t bumper;
    int32_t frameTimer;
    pbRect_t zones[NUM_ZONES];
} pinball_t;

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t pinballMode;
