/**
 * @file mode_2048.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief A game of 2048 for 2024-2025 Swadge hardware
 * @version 1.0
 * @date 2024-06-28
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

//==============================================================================
//  Version 1.1 goals:
//==============================================================================

/*
- LEDs that color based on highes score block and pulse during a merge
- Sounds
- Better splash screen
- High score saving
- Fix colors of blocks to e easily discerned
*/

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

typedef enum{
    GAME,       // Game running
    WIN,        // Display a win
    GAMEOVER,   // Display final score and prompt a restart
    GAMESTART,  // Splash screen after load
} DisplayState_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct{
    int32_t boardArr[4][4];
    uint32_t score;
    font_t font;
    font_t titleFont;
    char scoreStr[16];
    bool alreadyWon;
    DisplayState_t ds;
} t48_t;

//==============================================================================
// Function Prototypes
//==============================================================================

// Swadge functions

/**
 * @brief Mode setup
 * 
 */
static void t48EnterMode(void);

/**
 * @brief Mode teardown
 * 
 */
static void t48ExitMode(void);

/**
 * @brief Main loop of the code
 * 
 * @param elapsedUs 
 */
static void t48MainLoop(int64_t elapsedUs);

// Game functions

/**
 * @brief Sets an empty cell to 2 or 4 on 50/50 basis
 * 
 * @return int Cell used
 */
static int t48SetRandCell(void);

/**
 * @brief Merge a single row or column. Cells merged from CELL_SIZE-1 -> 0.
 * 
 * @param slice Slice to merge
 * @param updated If board was already updated
 * @return true If a merge occured
 * @return false if no merge occured
 */
static bool t48MergeSlice(int *slice, bool updated);

/**
 * @brief Slide blocks down if possible
 * 
 */
static void t48SlideDown(void);

/**
 * @brief Slide blocks up if possible
 * 
 */
static void t48SlideUp(void);

/**
 * @brief Slide blocks right if possible
 * 
 */
static void t48SlideRight(void);

/**
 * @brief Slide blocks left if possible
 * 
 */
static void t48SlideLeft(void);

// Game state

/**
 * @brief Initializes a game
 * 
 */
static void t48StartGame(void);

/**
 * @brief Checks if the player has reached 2048 for tyhee first time
 * 
 * @return true     If this is the first time 2048 has been hit
 * @return false    Otherwise
 */
static bool t48CheckWin(void);

/**
 * @brief Checks if the game can no longer be played
 * 
 * @return true     Display final score and reset
 * @return false    Continue playing
 */
static bool t48CheckOver(void);

// Visuals

/**
 * @brief Get the Color for a specific value
 * 
 * @param val       Value that requires a color for its square
 * @return uint8_t  Color of the value's square
 */
static uint8_t getColor(uint32_t val);

/**
 * @brief Draws the grid, score, and tiles
 * 
 */
static void t48Draw(void);

/**
 * @brief Shows the title upon booting into the mode
 * 
 * @param color Color of tyhe title text
 */
static void t48StartScreen(uint8_t color);

/**
 * @brief Draws the final score and prompts the player to play again
 * 
 * @param score The final value of the game
 */
static void t48DrawGameOverScreen(int64_t score);

/**
 * @brief Draw the win screen. It doesn't do anythiung else.
 * 
 */
static void t48DrawWinScreen(void);

//==============================================================================
// Extern variables
//==============================================================================

extern swadgeMode_t t48Mode;
