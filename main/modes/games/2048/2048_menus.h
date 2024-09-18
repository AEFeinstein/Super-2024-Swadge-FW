/**
 * @file 2048_menus.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Handles the initialization and display of non-game screens
 * @version 1.0.0
 * @date 2024-09-17
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include "mode_2048.h"

void t48_initStartScreen(t48_t* t48);
void t48_drawStartScreen(t48_t* t48, paletteColor_t color);
void t48_drawGameOverScreen(t48_t* t48, int64_t score, paletteColor_t pc);