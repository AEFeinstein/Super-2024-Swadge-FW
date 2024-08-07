#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

uint32_t readCircleFromFile(uint8_t* tableData, jsScene_t* scene);
void jsBallSimulate(jsBall_t* ball, float dt, vecFl_t gravity);
