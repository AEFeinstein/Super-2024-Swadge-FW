#pragma once

#include "mode_ray.h"

void rayEnemyBossMoveAnimate(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyBossGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
