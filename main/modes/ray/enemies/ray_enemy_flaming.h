#pragma once

#include "mode_ray.h"

void rayEnemyFlamingMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyFlamingGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet, int32_t damageDivide);
int32_t rayEnemyFlamingGetTimer(rayEnemy_t* enemy, rayEnemyTimerType_t type);
rayMapCellType_t rayEnemyFlamingGetBullet(rayEnemy_t* enemy);
