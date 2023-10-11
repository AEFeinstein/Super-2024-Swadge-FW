#pragma once

#include "mode_ray.h"

typedef void (*rayEnemyMoveAnimate_t)(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
typedef bool (*rayEnemyGetShot_t)(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);

typedef struct ray_enemy
{
    rayEnemyMoveAnimate_t moveAnimate;
    rayEnemyGetShot_t getShot;
} enemyFuncs_t;

void rayEnemiesMoveAnimate(ray_t* ray, uint32_t elapsedUs);
bool rayEnemyGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
