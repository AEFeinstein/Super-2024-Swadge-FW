#pragma once

#include "artillery.h"

bool artilleryGameInput(artilleryData_t* ad, buttonEvt_t evt);
void artilleryGameLoop(artilleryData_t* ad, uint32_t elapsedUs, bool barrelChanged);
void artillerySwitchToState(artilleryData_t* ad, artilleryGameState_t newState);