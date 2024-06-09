#ifndef _TILEMAP_BIGBUG_H_
#define _TILEMAP_BIGBUG_H_
//==============================================================================
// Includes
//==============================================================================
#include "swadge2024.h"
#include "typedef_bigbug.h"

//==============================================================================
// Constants
//==============================================================================
#define TILE_FIELD_WIDTH 32   //matches the level wsg graphic width
#define TILE_FIELD_HEIGHT 192 //matches the level wsg graphic height

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================
struct bb_tilemap_t
{
    wsg_t s1Wsg[32];      ///< The 1st variants of   soft foreground tiles
    wsg_t m1Wsg[32];      ///< The 1st variants of medium foreground tiles
    wsg_t h1Wsg[32];      ///< The 1st variants of   hard foreground tiles

    wsg_t mg1Wsg[32];     ///< The 1st variants of midground tiles

    wsg_t surfaceWsg;     ///< A graphic at the surface of the city dump
    wsg_t bgWsg;          ///< The paralax background for depth

    int8_t fgTiles[TILE_FIELD_WIDTH][TILE_FIELD_HEIGHT]; ///< The array of foreground tiles. The number is the dirt's health. 0 is air.
    bool mgTiles[TILE_FIELD_WIDTH][TILE_FIELD_HEIGHT]; ///< The array of midground tiles. True is tile, False is not.
};

//==============================================================================
// Prototypes
//==============================================================================
void bb_initializeTileMap(bb_tilemap_t* tilemap);
void bb_loadWsgs(bb_tilemap_t* tilemap);
void bb_drawTileMap(bb_tilemap_t* tilemap, rectangle_t* camera);
void bb_DrawForegroundCornerTile(bb_tilemap_t* tilemap, rectangle_t* camera, const uint8_t* idx_arr, uint32_t i, uint32_t j);
void bb_DrawMidgroundCornerTile( bb_tilemap_t* tilemap, rectangle_t* camera, const uint8_t* idx_arr, uint32_t i, uint32_t j);
wsg_t (*bb_GetWsgArrForForegroundCoord(bb_tilemap_t* tilemap, const uint32_t i, const uint32_t j))[32];

#endif