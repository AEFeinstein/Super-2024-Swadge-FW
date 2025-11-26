#pragma once

#include "artillery.h"

<<<<<<< HEAD
=======
#define MAX_TURNS 7

>>>>>>> origin/main
bool artilleryGameInput(artilleryData_t* ad, buttonEvt_t evt);
void artilleryGameLoop(artilleryData_t* ad, uint32_t elapsedUs, bool barrelChanged);
void artillerySwitchToGameState(artilleryData_t* ad, artilleryGameState_t newState);
void artilleryPassTurn(artilleryData_t* ad);
bool artilleryIsMyTurn(artilleryData_t* ad);
