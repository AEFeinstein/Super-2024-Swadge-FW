#ifndef _RAY_RENDERER_H_
#define _RAY_RENDERER_H_

#include <stdint.h>
#include <stdbool.h>

void castFloorCeiling(ray_t* ray, int16_t firstRow, int16_t lastRow);
void castWalls(ray_t* ray);
void castSprites(ray_t* ray);

#endif