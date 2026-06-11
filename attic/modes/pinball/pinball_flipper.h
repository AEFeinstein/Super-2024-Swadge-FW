#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

uint32_t readFlipperFromFile(uint8_t* tableData, pbScene_t* scene);
void pinballDrawFlipper(pbFlipper_t* flipper, vec_t* cameraOffset);
void pbFlipperSimulate(pbFlipper_t* flipper, float dt);
vecFl_t pbFlipperGetTip(pbFlipper_t* flipper);
