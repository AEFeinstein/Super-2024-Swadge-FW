/**
 * @file T48_logic.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Game Logic for 2048
 * @version 1.0.0
 * @date 2024-08-16
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include "mode_2048.h"
#include "T48_leds.h"
#include "T48_animations.h"

//==============================================================================
// Function Prototypes
//==============================================================================

/**
 * @brief Slide blocks down if possible
 *
 * @param t48 Main Game Object
 */
void t48SlideDown(t48_t* t48);

/**
 * @brief Slide blocks up if possible
 *
 * @param t48 Main Game Object
 */
void t48SlideUp(t48_t* t48);

/**
 * @brief Slide blocks right if possible
 *
 * @param t48 Main Game Object
 */
void t48SlideRight(t48_t* t48);

/**
 * @brief Slide blocks left if possible
 *
 * @param t48 Main Game Object
 */
void t48SlideLeft(t48_t* t48);

// Game state

/**
 * @brief Initializes a game
 *
 * @param t48 Main Game Object
 */
void t48StartGame(t48_t* t48);

/**
 * @brief Checks if the player has reached 2048 for the first time
 *
 * @param t48 Main Game Object
 * @return true     If this is the first time 2048 has been hit
 * @return false    Otherwise
 */
bool t48CheckWin(t48_t* t48);

/**
 * @brief Checks if the game can no longer be played
 *
 * @param t48 Main Game Object
 * @return true     Display final score and reset
 * @return false    Continue playing
 */
bool t48CheckOver(t48_t* t48);

/**
 * @brief Sorts the scores and swaps them into the correct place
 *
 * @param t48 Main Game Object
 */
void t48SortHighScores(t48_t* t48);
