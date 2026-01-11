#ifndef _SPRITE_H_
#define _SPRITE_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "wsg.h"
#include "aabb_utils_bigbug.h"

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    int16_t originX;
    int16_t originY;
    uint8_t numFrames;
    uint8_t brightnessLevels;
    wsg_t* frames; // Can hold 1 or more pointers to wsg's
    bool allocated;
} bb_sprite_t;

#endif