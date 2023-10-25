#pragma once

#include "mode_ray.h"

void rayEnemyNormalMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyNormalGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
int32_t rayEnemyNormalGetShotTimer(rayEnemy_t* enemy);
rayMapCellType_t rayEnemyNormalGetBullet(rayEnemy_t* enemy);
