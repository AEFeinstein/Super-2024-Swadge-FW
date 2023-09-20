#ifndef _RAY_SCRIPT_H_
#define _RAY_SCRIPT_H_

#include "mode_ray.h"

void loadScripts(ray_t* ray, const uint8_t* fileData, uint32_t fileSize, uint32_t caps);
void freeScripts(ray_t* ray);

#endif