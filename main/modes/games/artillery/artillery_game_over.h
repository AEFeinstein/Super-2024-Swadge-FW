#pragma once

#include "hdw-btn.h"
#include "artillery.h"

void artilleryGameOverInput(artilleryData_t* ad, buttonEvt_t* evt);
void artilleryGameOverLoop(artilleryData_t* ad, uint32_t elapsedUs);
