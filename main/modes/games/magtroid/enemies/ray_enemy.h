#pragma once

#include "mode_ray.h"
#include "ray_map.h"
#include "ray_object.h"
#include "ray_tex_manager.h"
#include "ray_player.h"

void rayEnemiesMoveAnimate(ray_t* ray, uint32_t elapsedUs);
void rayEnemyGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
void rayEnemyCheckCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY);
rayMapCellType_t enemyDropAfterDeath(rayEnemy_t* enemy);
rayMapCellType_t rayEnemyStandardItemDrop(void);
void rayCreateEnemy(ray_t* ray, rayMapCellType_t type, int32_t id, q24_8 x, q24_8 y);
