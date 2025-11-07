#pragma once

#include "hdw-btn.h"
#include "artillery.h"

void artilleryHelpInit(artilleryData_t* ad);
void artilleryHelpDeinit(artilleryData_t* ad);
void artilleryHelpInput(artilleryData_t* ad, buttonEvt_t* evt);
void artilleryHelpLoop(artilleryData_t* ad, uint32_t elapsedUs);
