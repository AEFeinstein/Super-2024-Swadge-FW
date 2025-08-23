#ifndef _MG_SPRITE_H_
#define _MG_SPRITE_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>
#include "wsg.h"
#include "aabb_utils.h"
#include "vector2d.h"

//==============================================================================
// Structs
//==============================================================================
/*typedef struct
{
    int16_t x;
    int16_t y;
} mgSpriteOrigin_t;*/

typedef struct
{
    wsg_t* wsg;
    const vec_t* origin;
    const box_t* hitBox;
} mgSprite_t;

///==============================================================================
// Constants
//==============================================================================
static const vec_t origin_8_8 =
{
    .x = 8,
    .y = 8
};

static const vec_t origin_15_15 =
{
    .x = 15,
    .y = 15
};

static const box_t box_16_16 = 
{
    .x0 = 0,
    .x1 = 15,
    .y0 = 0,
    .y1 = 15
};

static const box_t box_16_32 = 
{
    .x0 = 0,
    .x1 = 15,
    .y0 = 0,
    .y1 = 31
};

#endif
