#pragma once

#include "mode_ray.h"
#include "ray_map.h"
#include "ray_object.h"

void rayEnemiesMoveAnimate(ray_t* ray, uint32_t elapsedUs);
void rayEnemyGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
void rayEnemyCheckCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY);
