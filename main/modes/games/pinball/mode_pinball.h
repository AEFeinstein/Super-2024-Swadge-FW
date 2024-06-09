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

#define MAX_NUM_BALLS   512
#define MAX_NUM_WALLS   100
#define MAX_NUM_BUMPERS 10
#define MAX_NUM_TOUCHES 16
#define MAX_NUM_PADDLES 6

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
} pbShapeType_t;

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
    circle_t cPivot;      ///< The circle that the paddle pivots on
    circle_t cTip;        ///< The circle at the tip of the paddle
    line_t sideL;         ///< The left side of the paddle when pointing upward
    line_t sideR;         ///< The right side of the paddle when pointing upward
    int32_t length;       ///< The length of the paddle, from pivot center to tip center
    int32_t angle;        ///< The current angle of the paddle
    paletteColor_t color; ///< The color of the paddle
} pbPaddle_t;

typedef struct
{
    void* obj;
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
    pbPaddle_t* paddles;
    uint32_t numPaddles;
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
