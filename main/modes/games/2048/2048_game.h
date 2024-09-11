#pragma once

#include "mode_2048.h"

void t48_gameInit(t48_t* t48);
void t48_gameDraw(t48_t* t48, int32_t elapsedUs);
void t48_gameInput(t48_t* t48, buttonBit_t button);
