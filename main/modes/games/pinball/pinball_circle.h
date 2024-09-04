#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

#define PINBALL_RADIUS 8

uint32_t readCircleFromFile(uint8_t* tableData, jsScene_t* scene);
void jsBallSimulate(jsBall_t* ball, float dt, vecFl_t gravity);
void jsCircleTimer(jsCircle_t* circle, int32_t elapsedUs);
