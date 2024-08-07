#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

uint32_t readFlipperFromFile(uint8_t* tableData, jsScene_t* scene);
void pinballDrawFlipper(jsFlipper_t* flipper);
void jsFlipperSimulate(jsFlipper_t* flipper, float dt);
vecFl_t jsFlipperGetTip(jsFlipper_t* flipper);
