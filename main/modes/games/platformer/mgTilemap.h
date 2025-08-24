#ifndef _MG_TILEMAP_H_
#define _MG_TILEMAP_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "wsg.h"
#include "macros.h"
#include "mega_pulse_ex_typedef.h"
#include "mgWsgManager.h"
#include "mgEntityManager.h"
#include "hashMap.h"

//==============================================================================
// Constants
//==============================================================================
#define MG_TILEMAP_DISPLAY_WIDTH_PIXELS  280 // The screen size
#define MG_TILEMAP_DISPLAY_HEIGHT_PIXELS 240 // The screen size
#define MG_TILEMAP_DISPLAY_WIDTH_TILES   19  // The screen size in tiles + 1
#define MG_TILEMAP_DISPLAY_HEIGHT_TILES  16  // The screen size in tiles + 1

#define MG_TILESIZE                16
#define MG_TILESIZE_IN_POWERS_OF_2 4

#define MG_TILESET_SIZE 72

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
} mg_warp_t;
struct mgTilemap_t
{
    mgWsgManager_t* wsgManager;

    uint8_t* map;
    uint8_t mapWidth;
    uint8_t mapHeight;

    mg_warp_t warps[16];

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

    mgEntityManager_t* entityManager;

    uint8_t animationFrame;
    int16_t animationTimer;

    mgEntitySpawnData_t* entitySpawns;
    hashMap_t entitySpawnMap;
};

//==============================================================================
// Prototypes
//==============================================================================
void mg_initializeTileMap(mgTilemap_t* tilemap, mgWsgManager_t* wsgManager);
void mg_drawTileMap(mgTilemap_t* tilemap);
void mg_scrollTileMap(mgTilemap_t* tilemap, int16_t x, int16_t y);
void mg_drawTile(mgTilemap_t* tilemap, uint8_t tileId, int16_t x, int16_t y);
bool mg_loadMapFromFile(mgTilemap_t* tilemap, cnfsFileIdx_t name);
void mg_tileSpawnEntity(mgTilemap_t* tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty);
void mg_hashSpawnEntity(mgEntityManager_t* entityManager, mgEntitySpawnData_t* entitySpawnData);
uint8_t mg_getTile(mgTilemap_t* tilemap, uint8_t tx, uint8_t ty);
void mg_setTile(mgTilemap_t* tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId);
bool mg_isSolid(uint8_t tileId);
void mg_unlockScrolling(mgTilemap_t* tilemap);
bool mg_needsTransparency(uint8_t tileId);
void mg_freeTilemap(mgTilemap_t* tilemap);

#endif
