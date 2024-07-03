#pragma once

#include "mode_ray.h"

void rayEnemyHiddenMoveAnimate(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyHiddenGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
