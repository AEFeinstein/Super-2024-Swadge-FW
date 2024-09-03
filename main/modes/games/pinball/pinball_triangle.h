#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

uint32_t readTriangleFromFile(uint8_t* tableData, jsScene_t* scene);
void jsBlinkTriangles(jsScene_t* scene, float dt);
