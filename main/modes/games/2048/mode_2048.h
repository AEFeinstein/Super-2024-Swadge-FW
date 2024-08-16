/**
 * @file mode_2048.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.1.4
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
- Animations
    - Slide
    - Sparkles for merges
    - Zoom in for new
- Fix bugs in 1.1
- Refactor files into multiple files
*/

//==============================================================================
//  Version 1.3 goals:
//==============================================================================

/*
- Tilt controls
  - Control selection
  - Deadzone
  - Tilt and return to neutral to slide
  - Allow for tilt recalibration with B
- New, custom sounds
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
#define T48_GRID_SIZE  4
#define T48_BOARD_SIZE 16

// Graphics
#define T48_CELL_SIZE   50
#define T48_LINE_WEIGHT 4
#define T48_SIDE_MARGIN 30
#define T48_TOP_MARGIN  20
#define T48_TILE_COUNT  16
#define T48_MAX_MERGES  8
#define T48_MAX_SEQ     16

// High score
#define T48_HS_COUNT  5
#define T48_HS_KEYLEN 14

// LEDs
#define T48_LED_TIMER 32

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
} t48DisplayState_t;

typedef enum
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    ALL
} t48Direction_t;

typedef enum
{
    STATIC,
    MOVING,
    MOVED,
    MERGED,
    NEW
} t48CellStateEnum_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsg_t image;
    int16_t pos[2];
    uint8_t spd;
    int8_t dir;
} t48FallingBlock_t;

typedef struct
{
    int8_t x;
    int8_t y;
} t48CellCoors_t;

typedef struct
{
    wsg_t image;
    t48CellCoors_t grid;
    int8_t gridEnd;
    int8_t speed;
    uint8_t sequence;
    bool horizontal;
    int32_t value;
} t48SlidingTile_t;

typedef struct
{
    t48CellCoors_t incoming;
    t48CellCoors_t end;
    t48CellStateEnum_t state;
    int32_t value;
} t48CellState_t;

typedef struct
{
    uint32_t sliceVal;
    t48CellCoors_t cell;
} Slice_t;

typedef struct
{
    // Assets
    font_t font;
    font_t titleFont;
    font_t titleFontOutline;
    wsg_t tiles[T48_TILE_COUNT];
    midiFile_t bgm;
    midiFile_t click;

    // Game state
    int32_t boardArr[T48_GRID_SIZE][T48_GRID_SIZE]; // Row, Col
    int32_t score;
    int32_t highScore[T48_HS_COUNT];
    bool newHS;

    // Display
    char scoreStr[16];
    bool alreadyWon;
    char playerInitials[4];
    char hsInitials[T48_HS_COUNT][4];
    bool textEntryDone;
    t48DisplayState_t ds;
    uint8_t hue;
    t48SlidingTile_t slidingTiles[8]; // Max amount of sliding tiles
    t48CellState_t cellState[T48_BOARD_SIZE];
    int8_t globalAnim;

    // LEDs
    led_t leds[CONFIG_NUM_LEDS];

    // Audio
    bool bgmIsPlaying;

    // Start screen
    t48FallingBlock_t fb[T48_TILE_COUNT];
    bool startScrInitialized;
    int16_t timer;
} t48_t;

//==============================================================================
// Variables
//==============================================================================

static const char modeName[] = "2048";

static const char highScoreKey[T48_HS_COUNT][T48_HS_KEYLEN]
    = {"t48HighScore0", "t48HighScore1", "t48HighScore2", "t48HighScore3", "t48HighScore4"};
static const char highScoreInitialsKey[T48_HS_COUNT][T48_HS_KEYLEN] = {
    "t48HSInitial0", "t48HSInitial1", "t48HSInitial2", "t48HSInitial3", "t48HSInitial4",
};

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t t48Mode;
