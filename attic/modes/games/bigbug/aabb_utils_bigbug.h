#ifndef _AABB_UTILS_BIGBUG_H_
#define _AABB_UTILS_BIGBUG_H_

#include "typedef_bigbug.h"

#include <stdint.h>
#include <stdbool.h>
#include <palette.h>
#include <vector2d.h>
#include <vector2d.h>

typedef struct
{
    vec_t pos; // at the center
    int32_t halfWidth;
    int32_t halfHeight;
} bb_box_t;

bool bb_boxesCollide(bb_entity_t* unyielding, bb_entity_t* yielding, vec_t* previousPos, bb_hitInfo_t* hitInfo);

#endif