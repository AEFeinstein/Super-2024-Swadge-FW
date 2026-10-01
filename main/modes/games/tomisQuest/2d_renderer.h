#pragma once

#include <stdint.h>
#include "mode_ray.h"

void drawBackground2d(ray_t* ray, int32_t firstRow, int32_t lastRow);
void drawForeground2d(ray_t* ray);
void rayCenterCameraOnPlayer(ray_t* ray);
void update2dCamera(ray_t* ray, uint32_t elapsedUs);