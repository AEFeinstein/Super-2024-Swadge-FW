#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <esp_random.h>
#include "swadge2024.h"

//==============================================================================
// Defines
//==============================================================================

#define PIN_US_PER_FRAME 16667
#define NUM_ZONES        32

#define MAX_NUM_BALLS    512
#define MAX_NUM_WALLS    1024
#define MAX_NUM_BUMPERS  10
#define MAX_NUM_TOUCHES  16
#define MAX_NUM_FLIPPERS 6

#define NUM_FRAME_TIMES 60

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    PIN_NO_SHAPE,
    PIN_CIRCLE,
    PIN_LINE,
    PIN_RECT,
    PIN_FLIPPER,
} pbShapeType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    circleFl_t c;
    vecFl_t vel;     // Velocity is in pixels per frame (@ 60fps, so pixels per 16.7ms)
    vecFl_t accel;   // Acceleration is pixels per frame squared
    vecFl_t lastPos; // The previous postion, used to compare actual positional change to velocity
    bool bounce;     // true if the ball bounced this frame, false otherwise
    uint32_t zoneMask;
    paletteColor_t color;
    bool filled;
} pbCircle_t;

typedef struct
{
    lineFl_t l;
    float length;
    uint32_t zoneMask;
    paletteColor_t color;
} pbLine_t;

typedef struct
{
    rectangleFl_t r;
    uint32_t zoneMask;
    paletteColor_t color;
} pbRect_t;

typedef struct
{
    pbCircle_t cPivot; ///< The circle that the flipper pivots on
    pbCircle_t cTip;   ///< The circle at the tip of the flipper
    pbLine_t sideL;    ///< The left side of the flipper when pointing upward
    pbLine_t sideR;    ///< The right side of the flipper when pointing upward
    int32_t length;    ///< The length of the flipper, from pivot center to tip center
    float angle;       ///< The current angle of the flipper
    bool facingRight;  ///< True if the flipper is facing right, false if left
    bool buttonHeld;   ///< True if the button is being held down, false if it is released
    uint32_t zoneMask; ///< The zones this flipper is in
} pbFlipper_t;

typedef struct
{
    const void* obj;
    pbShapeType_t type;
} pbTouchRef_t;

typedef struct
{
    pbCircle_t* balls;
    uint32_t numBalls;
    pbTouchRef_t** ballsTouching;
    pbLine_t* walls;
    uint32_t numWalls;
    pbCircle_t* bumpers;
    uint32_t numBumpers;
    pbFlipper_t* flippers;
    uint32_t numFlippers;
    int32_t frameTimer;
    pbRect_t zones[NUM_ZONES];
    font_t ibm_vga8;

    uint32_t frameTimes[NUM_FRAME_TIMES];
    uint32_t frameTimesIdx;
} pinball_t;

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t pinballMode;
