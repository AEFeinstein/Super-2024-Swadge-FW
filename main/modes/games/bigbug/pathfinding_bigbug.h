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
    uint8_t x;
    uint8_t y;
    bool     z;//true for foreground false for midground
    uint16_t gCost;
    uint16_t hCost;
}bb_node_t;

//==============================================================================
// Prototypes
//==============================================================================
uint16_t fCost(bb_node_t* node);
bool pathfindToPerimeter(bb_node_t* start);


#endif