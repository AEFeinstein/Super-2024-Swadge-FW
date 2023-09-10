#ifndef _RAY_PLAYER_H_
#define _RAY_PLAYER_H_

#include "mode_ray.h"

void initializePlayer(ray_t* ray);
void rayPlayerCheckButtons(ray_t* ray, uint32_t elapsedUs);
void rayPlayerCheckJoystick(ray_t* ray, uint32_t elapsesUs);

#endif