#ifndef _PATHFINDING_BIGBUG_H_
#define _PATHFINDING_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================
#include "swadge2024.h"

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    uint8_t x;//an index in the tilemap
    uint8_t y;//an index in the tilemap
    bool z; // true for foreground false for midground
    uint16_t gCost;
    uint16_t hCost;
} bb_node_t;

//==============================================================================
// Prototypes
//==============================================================================
uint16_t fCost(const bb_node_t* node);
bool pathfindToPerimeter(bb_node_t* start);

#endif