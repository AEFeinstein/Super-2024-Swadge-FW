/**
 * @file 2048_game.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Core of 2048 mode
 * @version 1.0.0
 * @date 2024-09-17
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include "mode_2048.h"

void t48_gameInit(t48_t* t48, bool tiltControls);
void t48_gameLoop(t48_t* t48, int32_t elapsedUs);
void t48_gameInput(t48_t* t48, buttonBit_t button);
