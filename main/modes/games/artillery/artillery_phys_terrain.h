#pragma once

#include "artillery_phys.h"

void explodeShell(physSim_t* phys, node_t* shellNode);
bool moveTerrainPoints(physSim_t* phys, int32_t elapsedUs);
