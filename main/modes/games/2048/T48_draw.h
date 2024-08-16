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

/**
 * @brief Get the Color for a specific value
 *
 * @param val Value that requires a color for its square
 * @return uint8_t Color of the value's square
 */
static uint8_t getTileSprIndex(uint32_t val);

/**
 * @brief Draws the given tile onto the grid in the indicated position
 * 
 * @param t48 Main Game Object
 * @param img wsg to draw
 * @param row Row coordinate
 * @param col Column coordinate
 * @param xOff Offset of x coord
 * @param yOff Offset of y coord
 * @param val Value of the block
 */
static void t48DrawTileOnGrid(t48_t* t48, wsg_t* img, int8_t row, int8_t col, int16_t xOff, int16_t yOff, int32_t val);

/**
 * @brief 
 * 
 * @param t48 Main Game Object
 */
static void t48DrawCellState(t48_t* t48);

/**
 * @brief 
 * 
 * @param t48 Main Game Object
 */
static void t48DrawSlidingTiles(t48_t* t48);
