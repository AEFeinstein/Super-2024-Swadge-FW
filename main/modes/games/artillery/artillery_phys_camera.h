#pragma once

#include "artillery_phys.h"

void physSetCameraButton(physSim_t* phys, buttonBit_t btn);
bool physAdjustCamera(physSim_t* phys, uint32_t elapsedUs);
