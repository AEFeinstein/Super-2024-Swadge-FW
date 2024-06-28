/**
 * @file mode_2048.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2024-06-28
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"

//==============================================================================
// Defines
//==============================================================================

// Swadge
#define T48_US_PER_FRAME 16667

// Graphics
#define T48_CELL_SIZE   50
#define T48_LINE_WEIGHT 4
#define SIDE_MARGIN     30
#define TOP_MARGIN      20

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct{
    int32_t boardArr[4][4];
    uint32_t score;
    int16_t presses;
    font_t* font;
    char scoreStr[16];
} t48_t;

typedef struct{
    uint8_t color;
    int32_t val;
} colorPair_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void t48EnterMode(void);
static void t48ExitMode(void);
static void t48MainLoop(int64_t elapsedUs);
static void t48BGCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void t48Draw(void);

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t t48Mode;
