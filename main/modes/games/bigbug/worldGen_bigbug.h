#ifndef _WORLDGEN_BIGBUG_H_
#define _WORLDGEN_BIGBUG_H_
//==============================================================================
// Includes
//==============================================================================
#include "swadge2024.h"

//==============================================================================
// Prototypes
//==============================================================================

int bb_randomInt(int lowerBound, int upperBound);
void bb_initializeEggs(bb_entityManager_t* entityManager, bb_tilemap_t* tilemap);

#endif