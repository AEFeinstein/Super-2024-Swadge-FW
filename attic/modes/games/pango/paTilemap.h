#ifndef _PA_TILE_MAP_H_
#define _PA_TILE_MAP_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "wsg.h"
#include "paWsgManager.h"
#include "macros.h"
#include "pango_typedef.h"
#include "paEntityManager.h"

//==============================================================================
// Constants
//==============================================================================
#define PA_TILE_MAP_DISPLAY_WIDTH_PIXELS  280 // The screen size
#define PA_TILE_MAP_DISPLAY_HEIGHT_PIXELS 240 // The screen size
#define PA_TILE_MAP_DISPLAY_WIDTH_TILES   19  // The screen size in tiles + 1
#define PA_TILE_MAP_DISPLAY_HEIGHT_TILES  16  // The screen size in tiles + 1

#define PA_TILE_SIZE                16
#define PA_HALF_TILE_SIZE           8
#define PA_TILE_SIZE_IN_POWERS_OF_2 4

#define PA_TILE_SET_SIZE 15

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    uint8_t x;
    uint8_t y;
} pa_warp_t;
struct paTilemap_t
{
    paWsgManager_t* wsgManager;

    uint8_t* map;
    uint8_t mapWidth;
    uint8_t mapHeight;

    pa_warp_t warps[16];

    int16_t mapOffsetX;
    int16_t mapOffsetY;

    int16_t minMapOffsetX;
    int16_t maxMapOffsetX;
    int16_t minMapOffsetY;
    int16_t maxMapOffsetY;

    bool tileSpawnEnabled;
    int16_t executeTileSpawnColumn;
    int16_t executeTileSpawnRow;
    bool executeTileSpawnAll;

    paEntityManager_t* entityManager;
};

//==============================================================================
// Prototypes
//==============================================================================
void pa_initializeTileMap(paTilemap_t* tilemap, paWsgManager_t* wsgManager);
void pa_drawTileMap(paTilemap_t* tilemap);
void pa_scrollTileMap(paTilemap_t* tilemap, int16_t x, int16_t y);
void pa_drawTile(paTilemap_t* tilemap, uint8_t tileId, int16_t x, int16_t y);
bool pa_loadMapFromFile(paTilemap_t* tilemap, const char* name);
void pa_tileSpawnEntity(paTilemap_t* tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty);
uint8_t pa_getTile(paTilemap_t* tilemap, uint8_t tx, uint8_t ty);
void pa_setTile(paTilemap_t* tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId);
bool pa_isSolid(uint8_t tileId);
void pa_unlockScrolling(paTilemap_t* tilemap);
void pa_freeTilemap(paTilemap_t* tilemap);
void pa_generateMaze(paTilemap_t* tilemap);
bool pa_genPathContinue(paTilemap_t* tilemap, uint32_t x, uint32_t y);
void pa_genMakePath(paTilemap_t* tilemap, uint32_t x, uint32_t y);
void pa_placeEnemySpawns(paTilemap_t* tilemap);

#endif
