#pragma once

#include "mode_pinball.h"

void pbCreateBall(pinball_t* p, float x, float y);
void createRandomBalls(pinball_t* p, int32_t numBalls);
void createRandomWalls(pinball_t* p, int32_t numWalls);
void createRandomBumpers(pinball_t* p, int32_t numBumpers);

void createFlipper(pinball_t* p, int32_t pivot_x, int32_t pivot_y, int32_t pivot_r, int32_t tip_r, int32_t len,
                   bool facingRight);
void createWall(pinball_t* p, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void createBumper(pinball_t* p, int32_t x, int32_t y, int32_t r);

void loadConstPbTable(pinball_t* p);
