#pragma once

#include "mode_ray.h"

void rayEnemyNormalMoveAnimate(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyNormalGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
