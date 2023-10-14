#ifndef _RAY_WARP_SCREEN_H_
#define _RAY_WARP_SCREEN_H_

#include "mode_ray.h"

void drawWarpBackground(int16_t x, int16_t y, int16_t w, int16_t h);
void rayWarpScreenRender(ray_t* ray, uint32_t elapsedUs);
void setWarpDestination(ray_t* ray, int32_t mapId, int16_t posX, int16_t posY);
void warpToDestination(ray_t* ray);

#endif