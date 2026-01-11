#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

int32_t readLineFromFile(uint8_t* tableData, pbScene_t* scene);
void pinballDrawLine(pbLine_t* line, vec_t* cameraOffset);
void pbLineTimer(pbLine_t* line, int32_t elapsedUs, pbScene_t* scene);
