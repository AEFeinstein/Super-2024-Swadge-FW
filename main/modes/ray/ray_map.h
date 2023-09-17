#ifndef _RAY_MAP_H_
#define _RAY_MAP_H_

#include "mode_ray.h"

void loadRayMap(const char* name, ray_t* ray, bool spiRam);
void freeRayMap(rayMap_t* map);
bool isPassableCell(rayMapCell_t* cell);
void markTileVisited(rayMap_t* map, int16_t x, int16_t y);

#endif