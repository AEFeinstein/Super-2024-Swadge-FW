#pragma once

#include "artillery_phys.h"

#define TERRAIN_ITERATIONS 7
#define NUM_TERRAIN_POINTS ((1 << TERRAIN_ITERATIONS) + 1)

#define MAX_NUM_OBSTACLES 16

void physGenerateTerrain(physSim_t* phys, int32_t groundLevel);
void flattenTerrainUnderPlayer(physSim_t* phys, physCirc_t* player);
void explodeShell(physSim_t* phys, node_t* shellNode);
bool moveTerrainPoints(physSim_t* phys, int32_t elapsedUs);
void physAddTerrainPoints(physSim_t* phys, uint16_t tIdx, const uint16_t* terrainPoints, uint16_t numTerrainPoints);