#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

#define PINBALL_RADIUS 8

uint32_t readCircleFromFile(uint8_t* tableData, pbScene_t* scene);
void pbBallSimulate(pbBall_t* ball, int32_t elapsedUs, float dt, pbScene_t* scene);
void pbCircleTimer(pbCircle_t* circle, int32_t elapsedUs);
