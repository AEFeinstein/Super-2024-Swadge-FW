#ifndef _LIGHTING_BIGBUG_H_
#define _LIGHTING_BIGBUG_H_
//==============================================================================
// Includes
//==============================================================================
#include "swadge2024.h"
#include "vector2d.h"
#include "entityManager_bigbug.h"
#include "wsg.h"
#include <stdint.h>

//==============================================================================
// Prototypes
//==============================================================================
uint8_t bb_midgroundLighting(wsg_t* headlampWsg, vec_t* lookup, int32_t* garbotnikRotation);

uint8_t bb_foregroundLighting(wsg_t* headlampWsg, vec_t* lookup, int32_t* garbotnikRotation);

#endif