#ifndef _AABB_UTILS_BIGBUG_H_
#define _AABB_UTILS_BIGBUG_H_

#include <stdint.h>
#include <stdbool.h>
#include <palette.h>
#include <vector2d.h>

typedef struct
{
    vec_t pos;//at the center
    int32_t halfWidth;
    int32_t halfHeight;
} bb_box_t;

void bb_drawBox(bb_box_t* box, paletteColor_t* color, bool isFilled);
bool bb_boxesCollide(bb_box_t* box0, bb_box_t* box1);
bool bb_boxesCollideShift(bb_box_t* box0, bb_box_t* box1);

#endif