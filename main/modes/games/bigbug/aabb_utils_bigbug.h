#ifndef _AABB_UTILS_BIGBUG_H_
#define _AABB_UTILS_BIGBUG_H_

#include <stdint.h>
#include <stdbool.h>
#include <palette.h>

typedef struct
{
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;
} bb_box_t;

void bb_drawBox(bb_box_t box, paletteColor_t color, bool isFilled, int32_t scalingFactor);
bool bb_boxesCollide(bb_box_t box0, bb_box_t box1, int32_t scalingFactor);

#endif