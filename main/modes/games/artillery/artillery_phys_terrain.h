#pragma once

#include "artillery_phys.h"

void physGenerateTerrain(physSim_t* phys);
void flattenTerrainUnderPlayer(physSim_t* phys, physCirc_t* player);
void explodeShell(physSim_t* phys, node_t* shellNode);
bool moveTerrainPoints(physSim_t* phys, int32_t elapsedUs);
