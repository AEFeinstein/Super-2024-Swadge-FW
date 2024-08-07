#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

uint32_t readRectangleFromFile(uint8_t* tableData, jsScene_t* scene);
void jsLauncherSimulate(jsLauncher_t* launcher, jsBall_t* balls, int32_t numBalls, float dt);
