#pragma once

#include "swadge2024.h"
#include "mode_ray.h"

void initLoadedTextures(ray_t* ray);
uint8_t loadTexture(ray_t* ray, const char* name, rayMapCellType_t type);
wsg_t* getTexByType(ray_t* ray, rayMapCellType_t type);
wsg_t* getTexById(ray_t* ray, uint8_t id);
void freeAllTex(ray_t* ray);
