#pragma once

#include "mode_ray.h"

void rayEnemyStrongMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
bool rayEnemyStrongGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
int32_t rayEnemyStrongGetShotTimer(rayEnemy_t* enemy);
rayMapCellType_t rayEnemyStrongGetBullet(rayEnemy_t* enemy);
