/**
 * @file T48_draw.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Functions to draw the game board
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
 * @brief Draws the grid, score, and tiles
 *
 * @param t48 Main Game Object
 */
void t48Draw(t48_t* t48);