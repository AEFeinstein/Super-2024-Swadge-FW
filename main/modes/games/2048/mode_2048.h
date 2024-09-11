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
// Includes
//==============================================================================

#include <stdint.h>
#include <esp_random.h>

#include "swadge2024.h"
#include "textEntry.h"

//==============================================================================
// Defines
//==============================================================================

// Swadge
#define T48_US_PER_FRAME 16667

// Sprite counts
#define T48_TILE_COUNT   16
#define T48_MAX_SPARKLES 24

// Animations
#define T48_SPARKLE_COUNT 8

#define T48_GRID_SIZE      4
#define T48_TILES_PER_CELL 2

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint32_t value;
    int32_t xOffset;
    int32_t xOffsetTarget;
    int32_t yOffset;
    int32_t yOffsetTarget;
} t48drawnTile_t;

typedef struct
{
    int32_t value;
    t48drawnTile_t drawnTiles[T48_TILES_PER_CELL];
} t48cell_t;

typedef struct
{
    // Assets
    font_t font;
    font_t titleFont;
    font_t titleFontOutline;
    wsg_t tiles[T48_TILE_COUNT];
    wsg_t sparkleSprites[T48_SPARKLE_COUNT];
    midiFile_t bgm;
    midiFile_t click;

    // Game state
    t48cell_t board[T48_GRID_SIZE][T48_GRID_SIZE];
    int32_t score;
    bool acceptGameInput;
} t48_t;

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t t48Mode;
