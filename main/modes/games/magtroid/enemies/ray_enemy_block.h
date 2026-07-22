#pragma once

#include "mode_ray.h"

void rayEnemyBlockCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY);