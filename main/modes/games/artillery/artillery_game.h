#pragma once

#include "artillery.h"

#define MAX_TURNS 7

bool artilleryGameInput(artilleryData_t* ad, buttonEvt_t evt);
void artilleryGameLoop(artilleryData_t* ad, uint32_t elapsedUs, bool stateChanged);
void artillerySwitchToGameState(artilleryData_t* ad, artilleryGameState_t newState);
void artilleryPassTurn(artilleryData_t* ad);
bool artilleryIsMyTurn(artilleryData_t* ad);
void artilleryFinishTour(artilleryData_t* ad);