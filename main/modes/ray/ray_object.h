#ifndef _RAY_OBJECT_H_
#define _RAY_OBJECT_H_

#include "mode_ray.h"

void initEnemyTemplates(ray_t* ray);
void fireShot(ray_t* ray);
void moveRayObjects(ray_t* ray, int64_t elapsedUs);
void checkRayCollisions(ray_t* ray);

#endif
