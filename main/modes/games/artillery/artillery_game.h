#pragma once

#include "artillery.h"

void artilleryGameInput(artilleryData_t* ad, buttonEvt_t evt);
void artilleryGameLoop(artilleryData_t* ad, uint32_t elapsedUs);
void artillerySwitchToState(artilleryData_t* ad, artilleryGameState_t newState);