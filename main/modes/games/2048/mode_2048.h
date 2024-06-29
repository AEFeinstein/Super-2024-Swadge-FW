/**
 * @file mode_2048.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
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

#include <math.h>
#include <esp_random.h>

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
#define GRID_SIZE       4
#define BOARD_SIZE      16

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
    font_t font;
    char scoreStr[16];
    bool alreadyWon;
} t48_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void t48EnterMode(void);
static void t48ExitMode(void);
static void t48MainLoop(int64_t elapsedUs);
static int t48SetRandCell(void);
static void t48StartGame(void);
static bool t48CheckWin(void);
static bool t48CheckOver(void);
static void t48Draw(void);
static uint8_t getColor(uint32_t val);

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t t48Mode;
