#pragma once

#include "hdw-btn.h"
#include "artillery.h"

void artilleryHelpInput(artilleryData_t* ad, buttonEvt_t* evt);
void artilleryHelpLoop(artilleryData_t* ad, uint32_t elapsedUs);
