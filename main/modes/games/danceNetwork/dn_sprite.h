#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "wsg.h"

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
} dn_sprite_t;