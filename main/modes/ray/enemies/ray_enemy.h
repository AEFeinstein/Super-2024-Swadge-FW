#pragma once

#include "mode_ray.h"
#include "ray_map.h"
#include "ray_object.h"

void rayEnemiesMoveAnimate(ray_t* ray, uint32_t elapsedUs);
void rayEnemyGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
bool rayEnemyTransitionState(rayEnemy_t* enemy, rayEnemyState_t newState);
void switchEnemiesToXray(ray_t* ray, bool isXray);
int32_t getShotTimerForEnemy(rayEnemy_t* enemy);
rayMapCellType_t getBulletForEnemy(rayEnemy_t* enemy);
