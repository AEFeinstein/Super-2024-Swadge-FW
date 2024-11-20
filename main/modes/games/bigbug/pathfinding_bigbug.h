#ifndef _PATHFINDING_BIGBUG_H_
#define _PATHFINDING_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================
#include "swadge2024.h"
#include "typedef_bigbug.h"
#include "tilemap_bigbug.h"

//==============================================================================
// Structs
//==============================================================================

//==============================================================================
// Prototypes
//==============================================================================
uint16_t fCost(const bb_tileInfo_t* tile);
bool isPerimeterNode(const bb_tileInfo_t* tile);
void getNeighbors(const bb_tileInfo_t* tile, list_t* neighbors, bb_tilemap_t* tilemap, const bool toggleIteration);
bool contains(const list_t* nodeList, const bb_tileInfo_t* tile);
bool pathfindToPerimeter(bb_tileInfo_t* start, bb_tilemap_t* tilemap, const bool toggleIteration);

#endif