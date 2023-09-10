#pragma once

#include "swadge2024.h"
#include "mode_ray.h"

void initLoadedTextures(ray_t* ray);
wsg_t* loadTexture(ray_t* ray, const char* name, rayMapCellType_t type);
wsg_t* getTexByType(ray_t* ray, rayMapCellType_t type);
void freeAllTex(ray_t* ray);
