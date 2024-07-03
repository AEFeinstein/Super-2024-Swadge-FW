#pragma once

#include "mode_ray.h"

void rayEnemyFlamingMoveAnimate(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyFlamingGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
