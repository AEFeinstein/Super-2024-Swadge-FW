#pragma once

#include "mode_ray.h"

void rayEnemyArmoredMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
int32_t rayEnemyArmoredGetTimer(rayEnemy_t* enemy, rayEnemyTimerType_t type);
rayMapCellType_t rayEnemyArmoredGetBullet(rayEnemy_t* enemy);
