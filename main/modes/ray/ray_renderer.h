#ifndef _RAY_RENDERER_H_
#define _RAY_RENDERER_H_

#include <stdint.h>
#include <stdbool.h>

#define TEX_WIDTH  64
#define TEX_HEIGHT 64

void castFloorCeiling(ray_t* ray, int16_t firstRow, int16_t lastRow);
void castWalls(ray_t* ray);
rayObj_t* castSprites(ray_t* ray);
void drawHud(ray_t* ray);

#endif