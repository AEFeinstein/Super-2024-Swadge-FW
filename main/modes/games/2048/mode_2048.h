/**
 * @file mode_2048.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.1.2
 * @date 2024-06-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

//==============================================================================
//  Version 1.2 goals:
//==============================================================================

/*
- Tilt controls
  - Control selection
  - Deadzone
  - Tilt and return to neutral to slide
  - Allow for tilt recalibration with B
- RGB mode for text
- Refactor files into multiple files
*/

//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include <esp_random.h>

#include "swadge2024.h"
#include "textEntry.h"

//==============================================================================
// Defines
//==============================================================================

// Swadge
#define T48_US_PER_FRAME 16667

// Game
#define GRID_SIZE  4
#define BOARD_SIZE 16

// Graphics
#define T48_CELL_SIZE   50
#define T48_LINE_WEIGHT 4
#define SIDE_MARGIN     30
#define TOP_MARGIN      20
#define TILE_COUNT      16

// High score
#define HS_COUNT  5
#define HS_KEYLEN 14

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    GAME,      // Game running
    WIN,       // Display a win
    GAMEOVER,  // Display final score and prompt a restart
    GAMESTART, // Splash screen after load
    CONFIRM,   // Confirm player wants top abandon game
    WRITE,     // Allows player to write their initials for high score
} DisplayState_t;

typedef enum
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    ALL
} Direction_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color_t;

typedef struct
{
    wsg_t image;
    int16_t pos[2];
    uint8_t spd;
    int8_t dir;
} FallingBlock_t;

typedef struct
{
    // Assets
    font_t font;
    font_t titleFont;
    font_t titleFontOutline;
    wsg_t tiles[TILE_COUNT];
    midiFile_t bgm;
    midiFile_t click;

    // Game state
    int32_t boardArr[BOARD_SIZE][BOARD_SIZE];
    int32_t score;
    int32_t highScore[HS_COUNT];
    bool newHS;

    // Display
    char scoreStr[16];
    bool alreadyWon;
    char playerInitials[4];
    char hsInitials[HS_COUNT][4];
    bool textEntryDone;
    DisplayState_t ds;
    uint8_t hue;

    // Audio
    bool bgmIsPlaying;

    // Start screen
    FallingBlock_t fb[TILE_COUNT];
    bool startScrInitialized;
    int16_t timer;
} t48_t;

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t t48Mode;
