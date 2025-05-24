#ifndef _PL_TILEMAP_H_
#define _PL_TILEMAP_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "wsg.h"
#include "macros.h"
#include "platformer_typedef.h"
#include "plWsgManager.h"
#include "plEntityManager.h"

//==============================================================================
// Constants
//==============================================================================
#define PL_TILEMAP_DISPLAY_WIDTH_PIXELS  280 // The screen size
#define PL_TILEMAP_DISPLAY_HEIGHT_PIXELS 240 // The screen size
#define PL_TILEMAP_DISPLAY_WIDTH_TILES   19  // The screen size in tiles + 1
#define PL_TILEMAP_DISPLAY_HEIGHT_TILES  16  // The screen size in tiles + 1

#define PL_TILESIZE                16
#define PL_TILESIZE_IN_POWERS_OF_2 4

#define PL_TILESET_SIZE 72

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    uint8_t x;
    uint8_t y;
} pl_warp_t;
struct plTilemap_t
{
    plWsgManager_t* wsgManager;

    uint8_t* map;
    uint8_t mapWidth;
    uint8_t mapHeight;

    pl_warp_t warps[16];

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

    plEntityManager_t* entityManager;

    uint8_t animationFrame;
    int16_t animationTimer;
};

//==============================================================================
// Prototypes
//==============================================================================
void pl_initializeTileMap(plTilemap_t* tilemap, plWsgManager_t* wsgManager);
void pl_drawTileMap(plTilemap_t* tilemap);
void pl_scrollTileMap(plTilemap_t* tilemap, int16_t x, int16_t y);
void pl_drawTile(plTilemap_t* tilemap, uint8_t tileId, int16_t x, int16_t y);
bool pl_loadMapFromFile(plTilemap_t* tilemap, cnfsFileIdx_t name);
void pl_tileSpawnEntity(plTilemap_t* tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty);
uint8_t pl_getTile(plTilemap_t* tilemap, uint8_t tx, uint8_t ty);
void pl_setTile(plTilemap_t* tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId);
bool pl_isSolid(uint8_t tileId);
void pl_unlockScrolling(plTilemap_t* tilemap);
bool pl_needsTransparency(uint8_t tileId);
void pl_freeTilemap(plTilemap_t* tilemap);

#endif
