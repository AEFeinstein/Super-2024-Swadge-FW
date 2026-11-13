#ifndef _RAY_INSTRUMENT_H_
#define _RAY_INSTRUMENT_H_

#include "mode_ray.h"

extern const paletteColor_t noteColors[];

void rayInstrumentCheckButtons(ray_t* ray);
void rayInstrumentRender(ray_t* ray, uint32_t elapsedUs);

#endif