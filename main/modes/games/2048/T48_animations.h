/**
 * @file T48_animations.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Animation controller for 2048
 * @version 1.0.0
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

#include "mode_2048.h"

void t48ResetAnim(t48_t* t48);

void t48InitSparkles(t48_t* t48, int8_t idx, int8_t x, int8_t y, wsg_t* spr);

void t48InitMovingTilesVert(t48_t* t48);