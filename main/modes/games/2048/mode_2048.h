/**
 * @file mode_2048.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.5.0
 * @date 2024-06-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <esp_random.h>

#include "swadge2024.h"

//==============================================================================
// Defines
//==============================================================================

// Animations
#define T48_GRID_SIZE         4
#define T48_TILES_PER_CELL    2
#define T48_SPARKLES_PER_CELL 32

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    T48_IN_GAME,
    T48_START_SCREEN,
    T48_WIN_SCREEN,
    T48_END_SCREEN,
} t48ModeStateEnum_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint32_t value;  ///< The current value for the tile being drawn
    int32_t xOffset; ///< The X offset from the cell for this tile
    int32_t yOffset; ///< The Y offset from the cell for this tile
} t48drawnTile_t;

typedef struct
{
    wsg_t* img;   ///< A pointer to an image of a sparkle
    int16_t x;    ///< The sparkle's X coordinate (screen space)
    int16_t y;    ///< The sparkle's Y coordinate (screen space)
    int16_t xSpd; ///< The number of X pixels to move per-frame
    int16_t ySpd; ///< The number of Y pixels to move per-frame
    bool active;  ///< True if the spark is being animated and drawn
} t48Sparkle_t;

typedef struct
{
    int32_t value;                                 ///< This cell's current value
    t48drawnTile_t drawnTiles[T48_TILES_PER_CELL]; ///< All the tiles being drawn for this cell
    t48Sparkle_t sparkles[T48_SPARKLES_PER_CELL];  ///< All the sparkles being drawn for this cell
    bool merged;                                   ///< TODO
} t48cell_t;

typedef struct
{
    // Assets
    font_t font;             ///< Font used for tile values
    font_t titleFont;        ///< Font used for the title
    font_t titleFontOutline; ///< Font used for the title outline
    wsg_t* tiles;            ///< A list of tile sprites
    wsg_t* sparkleSprites;   ///< A list of sparkle sprites
    midiFile_t bgm;          ///< The background music
    midiFile_t click;        ///< The click sound

    // Game state
    t48cell_t board[T48_GRID_SIZE][T48_GRID_SIZE]; ///< The board with cells, tiles, and sparkles
    int32_t score;                                 ///< The current score
    bool acceptGameInput;                          ///< true if the game accepts input, false if it is animating
    bool paused;                                   ///< If the game is paused
    bool alreadyWon;                               ///< If the win screen has already displayed
    t48ModeStateEnum_t state;                      ///< Where in the game sequence we are

    // Audio
    bool bgmIsPlaying;
} t48_t;

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t t48Mode;
