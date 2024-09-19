#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

uint32_t readTriangleFromFile(uint8_t* tableData, pbScene_t* scene);
void pbTriangleTimer(pbTriangle_t* tri, int32_t elapsedUs);
