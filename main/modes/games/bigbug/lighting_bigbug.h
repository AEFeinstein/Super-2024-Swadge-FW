#ifndef _LIGHTING_BIGBUG_H_
#define _LIGHTING_BIGBUG_H_
//==============================================================================
// Includes
//==============================================================================
#include "swadge2024.h"
#include "vector2d.h"
#include "entityManager_bigbug.h"
#include <stdint.h>

//==============================================================================
// Prototypes
//==============================================================================
inline uint8_t bb_midgroundLighting(vec_t* lookup, bb_entityManager_t* entityManager);

inline uint8_t bb_foregroundLighting(vec_t* lookup, bb_entityManager_t* entityManager);

#endif