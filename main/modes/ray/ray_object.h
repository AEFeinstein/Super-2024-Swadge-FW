#ifndef _RAY_OBJECT_H_
#define _RAY_OBJECT_H_

#include "mode_ray.h"

void initEnemyTemplates(ray_t* ray);
void rayCreateBullet(ray_t* ray, rayMapCellType_t bulletType, q24_8 posX, q24_8 posY, q24_8 velX, q24_8 velY,
                     bool isPlayer);
void moveRayObjects(ray_t* ray, int32_t elapsedUs);
void checkRayCollisions(ray_t* ray);

#endif
