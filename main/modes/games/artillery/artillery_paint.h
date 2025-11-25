#pragma once

#include "artillery_types.h"

bool artilleryPaintLoadColor(artilleryData_t* ad);
void artilleryPaintInput(artilleryData_t* ad, buttonEvt_t* evt);
void artilleryPaintLoop(artilleryData_t* ad, uint32_t elapsedUs);
int32_t artilleryGetNumTankColors(void);
void artilleryGetTankColors(int32_t idx, paletteColor_t* base, paletteColor_t* accent);
void drawTank(int32_t x, int32_t y, int32_t r, paletteColor_t baseColor, paletteColor_t accentColor,
              int32_t barrelWidth);