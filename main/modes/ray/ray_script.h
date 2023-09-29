#ifndef _RAY_SCRIPT_H_
#define _RAY_SCRIPT_H_

#include "mode_ray.h"

void loadScripts(ray_t* ray, const uint8_t* fileData, uint32_t fileSize, uint32_t caps);
void freeScripts(ray_t* ray);

bool checkScriptShootObjs(ray_t* ray, int32_t id, wsg_t* portrait);
bool checkScriptKill(ray_t* ray, int32_t id, wsg_t* portrait);
bool checkScriptGet(ray_t* ray, int32_t id, wsg_t* portrait);
bool checkScriptTouch(ray_t* ray, int32_t id, wsg_t* portrait);
bool checkScriptShootWall(ray_t* ray, int32_t x, int32_t y);
bool checkScriptEnter(ray_t* ray, int32_t x, int32_t y);
bool checkScriptTime(ray_t* ray, uint32_t elapsedUs);

#endif