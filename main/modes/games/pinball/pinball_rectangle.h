#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

#define MAX_LAUNCHER_VELOCITY -600

uint32_t readRectangleFromFile(uint8_t* tableData, pbScene_t* scene);
void pbLauncherSimulate(pbLauncher_t* launcher, list_t* balls, float dt);
