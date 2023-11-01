#pragma once

#include "mode_ray.h"

void rayEnemyStrongMoveAnimate(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyStrongGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
