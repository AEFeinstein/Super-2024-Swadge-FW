#pragma once

#include "mode_pinball.h"

void createRandomBalls(pinball_t* p, int32_t numBalls);
void createRandomWalls(pinball_t* p, int32_t numWalls);
void createRandomBumpers(pinball_t* p, int32_t numBumpers);
void createPaddles(pinball_t* p, int32_t numPaddles);
