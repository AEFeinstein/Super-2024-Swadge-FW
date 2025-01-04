/**
 * @file mode_2048.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.5.1
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

// Animation
#define T48_NEW_SPARKLE_SEQ     33333
#define T48_START_SCREEN_BLOCKS 16

// High Scores
#define T48_HS_COUNT  5
#define T48_HS_KEYLEN 14

// LEDs
#define T48_FADE_SPEED 66666
#define T48_NEXT_LED   100000

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    T48_IN_GAME,
    T48_START_SCREEN,
    T48_LOAD_SAVE,
    T48_WIN_SCREEN,
    T48_HS_SCREEN,
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
    bool merged;                                   ///< If tile was merged
} t48cell_t;

typedef struct
{
    vec_t pos;         ///< x and y coordinates of the new tile
    int32_t spawnTime; ///< Time since the object was created
    bool active;       ///< If the new tile should be drawing anything
    int8_t sequence;   ///< Current position in the sequence
} t48newTile_t;

typedef struct
{
    vec_t pos;
    vec_t timer;
    vec_t speed;
    vec_t dir;
} t48StartScreenBlocks_t;

typedef struct
{
    int32_t board[T48_GRID_SIZE][T48_GRID_SIZE]; ///< Board state
    int32_t score;                               ///< Score when saved
} t48GameSaveData_t;

typedef struct
{
    // Assets
    font_t font;             ///< Font used for tile values
    font_t titleFont;        ///< Font used for the title
    font_t titleFontOutline; ///< Font used for the title outline
    wsg_t* tiles;            ///< A list of tile sprites
    wsg_t* sparkleSprites;   ///< A list of sparkle sprites
    wsg_t* newSparkles;      ///< New sparkles for a new tile
    midiFile_t bgm;          ///< The background music
    midiFile_t click;        ///< The click sound

    // Game state
    t48cell_t board[T48_GRID_SIZE][T48_GRID_SIZE]; ///< The board with cells, tiles, and sparkles
    int32_t score;                                 ///< The current score
    bool acceptGameInput;                          ///< true if the game accepts input, false if it is animating
    bool tiltControls;                             ///< true if tilt controls are used instead of buttons
    bool cellsAnimating;                           ///< true if cells are in motion, false if at rest
    bool paused;                                   ///< If the game is paused
    bool alreadyWon;                               ///< If the win screen has already displayed
    t48ModeStateEnum_t state;                      ///< Where in the game sequence we are

    // Input for IMU-based 2048.
    // Only used if tiltControls == true
    float quatBase[4];
    char receivedInputMask;
    int lastIMUx, lastIMUy;

    // Audio
    bool bgmIsPlaying; ///< Allows the BGM to restart

    // High Score
    char playerInitials[4];           ///< Contains the play initials
    char hsInitials[T48_HS_COUNT][4]; ///< Contains all the high score initials
    bool textEntryDone;               ///< Tested when checking if text entry is finished
    int32_t highScore[T48_HS_COUNT];  ///< Array of high scores

    // Start screen
    uint8_t hue; ///< Color of the title text

    // Animations
    t48newTile_t nTile;                                       ///< New tile animation data
    t48StartScreenBlocks_t ssBlocks[T48_START_SCREEN_BLOCKS]; ///< Start screen falling blocks

    // LEDs
    led_t leds[CONFIG_NUM_LEDS]; ///< LEDs to set
    int32_t fadeTimer;           ///< Timer between fades
    int32_t nextLedTimer;        ///< Timer before the next LED illuminates
    uint8_t currLED;             ///< Index of the led for the chase mode

    // Save game data
    t48GameSaveData_t sd; ///< Save data for temp saves
} t48_t;

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t t48Mode;
extern const char t48Name[];
