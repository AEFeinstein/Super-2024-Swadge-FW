#ifndef _RAY_PAUSE_H_
#define _RAY_PAUSE_H_

#include "mode_ray.h"

void rayShowPause(ray_t* ray);
void rayPauseCheckButtons(ray_t* ray);
void rayPauseRender(ray_t* ray, uint32_t elapsedUs);

#endif