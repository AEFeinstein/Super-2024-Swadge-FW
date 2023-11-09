#pragma once

#include "mode_ray.h"

void rayEnemyNormalMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyNormalGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet, int32_t damageDivide);
int32_t rayEnemyNormalGetTimer(rayEnemy_t* enemy, rayEnemyTimerType_t type);
rayMapCellType_t rayEnemyNormalGetBullet(rayEnemy_t* enemy);
