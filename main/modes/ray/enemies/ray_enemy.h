#pragma once

#include "mode_ray.h"
#include "ray_map.h"
#include "ray_object.h"

typedef void (*rayEnemyMove_t)(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
typedef bool (*rayEnemyGetShot_t)(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);

typedef struct ray_enemy
{
    rayEnemyMove_t move;
    rayEnemyGetShot_t getShot;
} enemyFuncs_t;

void rayEnemiesMoveAnimate(ray_t* ray, uint32_t elapsedUs);
void rayEnemyGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
void rayEnemyTransitionState(rayEnemy_t* enemy, rayEnemyState_t newState);
