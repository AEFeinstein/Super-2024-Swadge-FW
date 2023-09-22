#ifndef _PL_TILEMAP_H_
#define _PL_TILEMAP_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "wsg.h"
#include "platformer_typedef.h"
#include "entityManager.h"

//==============================================================================
// Constants
//==============================================================================
#define CLAMP(x, l, u) ((x) < l ? l : ((x) > u ? u : (x)))

#define TILEMAP_DISPLAY_WIDTH_PIXELS 280  // The screen size
#define TILEMAP_DISPLAY_HEIGHT_PIXELS 240 // The screen size
#define TILEMAP_DISPLAY_WIDTH_TILES 19    // The screen size in tiles + 1
#define TILEMAP_DISPLAY_HEIGHT_TILES 16   // The screen size in tiles + 1

#define TILE_SIZE 16
#define TILE_SIZE_IN_POWERS_OF_2 4

#define TILESET_SIZE 72

//==============================================================================
// Enums
//==============================================================================
typedef enum {
    TILE_EMPTY,
    TILE_WARP_0,
    TILE_WARP_1,
    TILE_WARP_2,
    TILE_WARP_3,
    TILE_WARP_4,
    TILE_WARP_5,
    TILE_WARP_6,
    TILE_WARP_7,
    TILE_WARP_8,
    TILE_WARP_9,
    TILE_WARP_A,
    TILE_WARP_B,
    TILE_WARP_C,
    TILE_WARP_D,
    TILE_WARP_E,
    TILE_WARP_F,
    TILE_CTNR_COIN,
    TILE_CTNR_10COIN,
    TILE_CTNR_POW1,
    TILE_CTNR_POW2,
    TILE_CTNR_LADDER,
    TILE_CTNR_1UP,
    TILE_CTRL_LEFT,
    TILE_CTRL_RIGHT,
    TILE_CTRL_UP,
    TILE_CTRL_DOWN,
    TILE_UNUSED_27,
    TILE_UNUSED_28,
    TILE_UNUSED_29,
    TILE_INVISIBLE_BLOCK,
    TILE_INVISIBLE_CONTAINER,
    TILE_GRASS,
    TILE_GROUND,
    TILE_BRICK_BLOCK,
    TILE_BLOCK,
    TILE_METAL_BLOCK,
    TILE_METAL_PIPE_H,
    TILE_METAL_PIPE_V,
    TILE_BOUNCE_BLOCK,
    TILE_DIRT_PATH,
    TILE_GIRDER,
    TILE_SOLID_UNUSED_42,
    TILE_SOLID_UNUSED_43,
    TILE_SOLID_UNUSED_44,
    TILE_SOLID_UNUSED_45,
    TILE_SOLID_UNUSED_46,
    TILE_SOLID_UNUSED_47,
    TILE_SOLID_UNUSED_48,
    TILE_SOLID_UNUSED_49,
    TILE_SOLID_UNUSED_50,
    TILE_SOLID_UNUSED_51,
    TILE_SOLID_UNUSED_52,
    TILE_SOLID_UNUSED_53,
    TILE_SOLID_UNUSED_54,
    TILE_SOLID_UNUSED_55,
    TILE_SOLID_UNUSED_56,
    TILE_SOLID_UNUSED_57,
    TILE_SOLID_UNUSED_58,
    TILE_GOAL_100PTS,
    TILE_GOAL_500PTS,
    TILE_GOAL_1000PTS,
    TILE_GOAL_2000PTS,
    TILE_GOAL_5000PTS,
    TILE_CONTAINER_1,
    TILE_CONTAINER_2,
    TILE_CONTAINER_3,
    TILE_COIN_1,
    TILE_COIN_2,
    TILE_COIN_3,
    TILE_LADDER,
    TILE_NONSOLID_INTERACTIVE_VISIBLE_71,
    TILE_NONSOLID_INTERACTIVE_VISIBLE_72,
    TILE_NONSOLID_INTERACTIVE_VISIBLE_73,
    TILE_NONSOLID_INTERACTIVE_VISIBLE_74,
    TILE_NONSOLID_INTERACTIVE_VISIBLE_75,
    TILE_NONSOLID_INTERACTIVE_VISIBLE_76,
    TILE_NONSOLID_INTERACTIVE_VISIBLE_77,
    TILE_NONSOLID_INTERACTIVE_VISIBLE_78,
    TILE_NONSOLID_INTERACTIVE_VISIBLE_79,
    TILE_BG_GOAL_ZONE,
    TILE_BG_ARROW_L,
    TILE_BG_ARROW_R,
    TILE_BG_ARROW_U,
    TILE_BG_ARROW_D,
    TILE_BG_ARROW_LU,
    TILE_BG_ARROW_RU,
    TILE_BG_ARROW_LD,
    TILE_BG_ARROW_RD,
    TILE_BG_CLOUD_LD,
    TILE_BG_CLOUD_M,
    TILE_BG_CLOUD_RD,
    TILE_BG_CLOUD_LU,
    TILE_BG_CLOUD_RU,
    TILE_BG_CLOUD_D,
    TILE_BG_CLOUD,
    TILE_BG_TALL_GRASS,
    TILE_BG_MOUNTAIN_L,
    TILE_BG_MOUNTAIN_U,
    TILE_BG_MOUNTAIN_R,
    TILE_BG_MOUNTAIN,
    TILE_BG_METAL,
    TILE_BG_CHAINS,
    TILE_BG_WALL
} tileIndex_t;

//==============================================================================
// Structs
//==============================================================================
typedef struct {
    uint8_t x;
    uint8_t y;
} warp_t;
 struct tilemap_t
{
    wsg_t tiles[TILESET_SIZE];

    uint8_t * map;
    uint8_t mapWidth;
    uint8_t mapHeight;
    
    warp_t warps[16];

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

    entityManager_t *entityManager;

    uint8_t animationFrame;
    int16_t animationTimer;
};

//==============================================================================
// Prototypes
//==============================================================================
void initializeTileMap(tilemap_t * tilemap);
void drawTileMap(tilemap_t * tilemap);
void scrollTileMap(tilemap_t * tilemap, int16_t x, int16_t y);
void drawTile(tilemap_t * tilemap, uint8_t tileId, int16_t x, int16_t y);
bool loadMapFromFile(tilemap_t * tilemap, const char * name);
bool loadTiles(tilemap_t * tilemap);
void tileSpawnEntity(tilemap_t * tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty);
uint8_t getTile(tilemap_t *tilemap, uint8_t tx, uint8_t ty);
void setTile(tilemap_t *tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId);
bool isSolid(uint8_t tileId);
void unlockScrolling(tilemap_t *tilemap);
bool needsTransparency(uint8_t tileId);
void freeTilemap(tilemap_t *tilemap);

#endif
