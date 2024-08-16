/**
 * @file T48_Menus.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Menu screens for 2048
 * @version 1.0.0
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include "mode_2048.h"

//==============================================================================
// Function Prototypes
//==============================================================================

/**
 * @brief Shows the title upon booting into the mode
 *
 * @param t48 Main Game Object
 * @param color Color of the title text
 */
void t48StartScreen(t48_t* t48, paletteColor_t color);

/**
 * @brief Draws the final score and prompts the player to play again
 *
 * @param t48 Main Game Object
 * @param score The final value of the game
 */
void t48DrawGameOverScreen(t48_t* t48, int64_t score, paletteColor_t pc);

/**
 * @brief Draw the win screen. It doesn't do anything else.
 *
 * @param t48 Main Game Object
 */
void t48DrawWinScreen(t48_t* t48);

/**
 * @brief Draw the screen asking to confirm they want to quit
 *
 * @param t48 Main Game Object
 */
void t48DrawConfirm(t48_t* t48);

//==============================================================================
// Variables
//==============================================================================

static const char pressKey[]   = "Press any key to play";
static const char pressAB[]    = "Press A or B to reset the game";
static const char youWin[]     = "You got 2048!";
static const char continueAB[] = "Press A or B to continue";
static const char highScore[]  = "You got a high score!";
static const char paused[]     = "Paused!";
static const char pausedA[]    = "Press A to continue playing";
static const char pausedB[]    = "Press B to abandon game";