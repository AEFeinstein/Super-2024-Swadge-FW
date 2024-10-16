#ifndef _AABB_UTILS_BIGBUG_H_
#define _AABB_UTILS_BIGBUG_H_

#include "typedef_bigbug.h"

#include <stdint.h>
#include <stdbool.h>
#include <palette.h>
#include <vector2d.h>

typedef struct
{
    vec_t pos; // at the center
    int32_t halfWidth;
    int32_t halfHeight;
} bb_box_t;

bool bb_boxesCollide(bb_entity_t* unmoving, bb_entity_t* moving, bb_hitInfo_t* hitInfo, vec_t* previousPos);

#endif