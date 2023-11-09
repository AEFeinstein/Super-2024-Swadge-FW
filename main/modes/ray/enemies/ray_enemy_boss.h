#pragma once

#include "mode_ray.h"

void rayEnemyBossMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyBossGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet, int32_t damageDivide);
int32_t rayEnemyBossGetTimer(rayEnemy_t* enemy, rayEnemyTimerType_t type);
rayMapCellType_t rayEnemyBossGetBullet(rayEnemy_t* enemy);
