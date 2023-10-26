#pragma once

#include "mode_ray.h"

void rayEnemyHiddenMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyHiddenGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
int32_t rayEnemyHiddenGetTimer(rayEnemy_t* enemy, rayEnemyTimerType_t type);
rayMapCellType_t rayEnemyHiddenGetBullet(rayEnemy_t* enemy);
