#pragma once

#include "pinball_typedef.h"
#include "pinball_game.h"

int32_t readLineFromFile(uint8_t* tableData, jsScene_t* scene);
void pinballDrawLine(jsLine_t* line);