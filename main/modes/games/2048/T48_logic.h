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

//==============================================================================
// Function Prototypes
//==============================================================================

/**
 * @brief Sets an empty cell to 2 or 4 on 50/50 basis
 *
 * @param t48 Main Game Object
 * @return int8_t Cell used
 */
int8_t t48SetRandCell(t48_t* t48);

/**
 * @brief Updates the LEDs based on a direction and adds a new tile if valid
 *
 * @param t48 Main Game Object
 * @param wasUpdated If the board has changed to a new config
 * @param dir Direction used to update board
 */
void t48BoardUpdate(t48_t* t48, bool wasUpdated, t48Direction_t dir);

/**
 * @brief Merge a single row or column. Cells merged from CELL_SIZE-1 -> 0.
 *
 * @param t48 Main Game Object
 * @param slice Slice to merge
 * @param updated If board was already updated
 * @return true If a merge occurred
 * @return false if no merge occurred
 */
void t48MergeSlice(t48_t* t48, bool* updated);

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

/* void t48ResetCellState(t48_t* t48);

void t48SetCellState(t48_t* t48, int8_t idx, t48CellStateEnum_t st, int8_t startX, int8_t startY, int8_t endX,
                     int8_t endY, int32_t value);

void t48UpdateCells(t48_t* t48);

void t48SetSlidingTile(t48_t* t48, int8_t idx, t48Cell_t start, t48Cell_t end, int32_t value); */