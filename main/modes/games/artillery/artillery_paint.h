#pragma once

#include "hdw-btn.h"
#include "artillery.h"

void artilleryPaintLoadColor(artilleryData_t* ad);
void artilleryPaintInput(artilleryData_t* ad, buttonEvt_t* evt);
void artilleryPaintLoop(artilleryData_t* ad, uint32_t elapsedUs);
int32_t artilleryGetNumTankColors(void);
void artilleryGetTankColors(int32_t idx, paletteColor_t* base, paletteColor_t* accent);
