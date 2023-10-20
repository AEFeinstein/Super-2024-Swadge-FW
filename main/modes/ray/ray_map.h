#ifndef _RAY_MAP_H_
#define _RAY_MAP_H_

#include "mode_ray.h"

void loadRayMap(const char* name, ray_t* ray, q24_8* pStartX, q24_8* pStartY, bool spiRam);
void freeRayMap(rayMap_t* map);
bool isPassableCell(rayMapCell_t* cell);
void markTileVisited(rayMap_t* map, int16_t x, int16_t y);
void rayCreateEnemy(ray_t* ray, rayMapCellType_t type, int32_t id, int32_t x, int32_t y);
void rayCreateCommonObj(ray_t* ray, rayMapCellType_t type, int32_t id, int32_t x, int32_t y);

#endif