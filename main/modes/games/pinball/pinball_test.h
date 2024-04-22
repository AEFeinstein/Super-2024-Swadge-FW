#pragma once

#include "mode_pinball.h"

void pbCreateBall(pinball_t* p, q24_8 x, q24_8 y);
void createRandomBalls(pinball_t* p, int32_t numBalls);
void createRandomWalls(pinball_t* p, int32_t numWalls);
void createRandomBumpers(pinball_t* p, int32_t numBumpers);
void createFlipper(pinball_t* p, int32_t pivot_x, int32_t pivot_y, bool facingRight);
