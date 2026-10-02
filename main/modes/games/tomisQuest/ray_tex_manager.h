#pragma once

#include "swadge.h"
#include "mode_ray.h"

void loadEnvTextures(ray_t* ray);
wsg_t* loadTexture(ray_t* ray, cnfsFileIdx_t fIdx, rayMapCellType_t type);
wsg_t* getTexByType(ray_t* ray, rayMapCellType_t type);
void freeAllTex(ray_t* ray);
