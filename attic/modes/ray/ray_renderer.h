#ifndef _RAY_RENDERER_H_
#define _RAY_RENDERER_H_

#include <stdint.h>
#include <stdbool.h>

#define TEX_WIDTH  64
#define TEX_HEIGHT 64

void runEnvTimers(ray_t* ray, uint32_t elapsedUs);

void castFloorCeiling(ray_t* ray, int32_t firstRow, int32_t lastRow);
void castWalls(ray_t* ray);
rayObjCommon_t* castSprites(ray_t* ray, rayEnemy_t** closestEnemy);
void drawHud(ray_t* ray);
void rayLightLeds(ray_t* ray, rayEnemy_t* closestEnemy, uint32_t elapsedUs);

#endif