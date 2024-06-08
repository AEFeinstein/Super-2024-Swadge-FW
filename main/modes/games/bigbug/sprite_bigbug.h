#ifndef _SPRITE_H_
#define _SPRITE_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>
#include "wsg.h"
#include "aabb_utils_bigbug.h"

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    wsg_t wsg;
    int16_t originX;
    int16_t originY;
    bb_box_t collisionBox;
} bb_sprite_t;

#endif