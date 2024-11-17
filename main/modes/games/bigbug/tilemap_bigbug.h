#ifndef _TILEMAP_BIGBUG_H_
#define _TILEMAP_BIGBUG_H_
//==============================================================================
// Includes
//==============================================================================
#include "swadge2024.h"
#include "typedef_bigbug.h"
#include "entityManager_bigbug.h"

//==============================================================================
// Constants
//==============================================================================
#define TILE_FIELD_WIDTH  74  // matches the level wsg graphic width
#define TILE_FIELD_HEIGHT 197 // matches the level wsg graphic height

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================
struct bb_tileInfo_t
{
    int8_t health; // 0 is air, > 0 is garbage. Use an int8_t incase damage decrements lower than zero, then we can snap
                   // to zero.
    bb_embeddable_t embed; // Some kind of something embedded in the garbage tile.
    bb_entity_t* entity;   // Null when the the player player is far away.
                           // Can become null again if the entity is still embed & player leaving.
};

struct bb_tilemap_t
{
    wsg_t headlampWsg; ///< Some data about lighting. Red channel is left facing. Green channel is center facing. Blue
                       ///< channel is right facing.

    wsg_t mid_s_Wsg[120]; ///< The soft   midground tiles.
    wsg_t mid_m_Wsg[120]; ///< The medium midground tiles.
    wsg_t mid_h_Wsg[120]; ///< The hard   midground tiles.

    wsg_t fore_s_Wsg[240]; ///< The soft   foreground tiles
    wsg_t fore_m_Wsg[240]; ///< The medium foreground tiles
    wsg_t fore_h_Wsg[240]; ///< The hard   foreground tiles
    wsg_t fore_b_Wsg[240]; ///< The brick  foreground tiles

    wsg_t surface1Wsg; ///< A graphic at the surface of the city dump
    wsg_t surface2Wsg; ///< A graphic at the surface of the city dump
    wsg_t bgWsg;       ///< The paralax background for depth

    bb_tileInfo_t fgTiles[TILE_FIELD_WIDTH][TILE_FIELD_HEIGHT]; ///< The array of foreground tiles. The number is the
                                                                ///< dirt's health. 0 is air.
    int8_t mgTiles[TILE_FIELD_WIDTH][TILE_FIELD_HEIGHT];        ///< The array of midground tiles.
};

struct bb_hitInfo_t
{
    bool hit;
    vec_t pos; // the precise hit point somewhere on an edge of a tile.
    vec_t normal;
    int32_t tile_i;
    int32_t tile_j;
};

//==============================================================================
// Prototypes
//==============================================================================
void bb_initializeTileMap(bb_tilemap_t* tilemap);
void bb_loadWsgs(bb_tilemap_t* tilemap);
void bb_freeWsgs(bb_tilemap_t* tilemap);
void bb_drawTileMap(bb_tilemap_t* tilemap, rectangle_t* camera, vec_t* garbotnikDrawPos, vec_t* garbotnikRotation,
                    bb_entityManager_t* entityManager);
void bb_DrawForegroundCornerTile(bb_tilemap_t* tilemap, rectangle_t* camera, const uint8_t* idx_arr, uint32_t i,
                                 uint32_t j);
void bb_DrawMidgroundCornerTile(bb_tilemap_t* tilemap, rectangle_t* camera, const uint8_t* idx_arr, uint32_t i,
                                uint32_t j);
void bb_collisionCheck(bb_tilemap_t* tilemap, bb_entity_t* ent, vec_t* previousPos, bb_hitInfo_t* hitInfo);
wsg_t (*bb_GetMidgroundWsgArrForCoord(bb_tilemap_t* tilemap, const uint32_t i, const uint32_t j))[120];
wsg_t (*bb_GetForegroundWsgArrForCoord(bb_tilemap_t* tilemap, const uint32_t i, const uint32_t j))[240];

#endif