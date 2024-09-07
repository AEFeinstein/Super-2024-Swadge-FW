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

void t48InitSparkles(t48_t* t48, int8_t idx, int8_t x, int8_t y, wsg_t spr);

void t48InitMovingTiles(t48_t* t48);

/* void t48ResetCellState(t48_t* t48);

void t48SetCellState(t48_t* t48, int8_t idx, t48CellStateEnum_t st, int8_t startX, int8_t startY, int8_t endX,
                     int8_t endY, int32_t value);

void t48UpdateCells(t48_t* t48);

void t48SetSlidingTile(t48_t* t48, int8_t idx, t48Cell_t start, t48Cell_t end, int32_t value); */