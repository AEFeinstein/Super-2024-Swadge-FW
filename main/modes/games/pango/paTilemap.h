#ifndef _PA_TILE_MAP_H_
#define _PA_TILE_MAP_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "wsg.h"
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
// Enums
//==============================================================================
typedef enum
{
    PA_TILE_EMPTY,
    PA_TILE_WALL_0,
    PA_TILE_WALL_1,
    PA_TILE_WALL_2,
    PA_TILE_WALL_3,
    PA_TILE_WALL_4,
    PA_TILE_WALL_5,
    PA_TILE_WALL_6,
    PA_TILE_WALL_7,
    PA_TILE_BLOCK,
    PA_TILE_SPAWN_BLOCK_0,
    PA_TILE_SPAWN_BLOCK_1,
    PA_TILE_SPAWN_BLOCK_2,
    PA_TILE_BONUS_BLOCK_0,
    PA_TILE_BONUS_BLOCK_1,
    PA_TILE_BONUS_BLOCK_2,
    PA_TILE_WARP_F,
    PA_TILE_CTNR_COIN,
    PA_TILE_CTNR_10COIN,
    PA_TILE_CTNR_POW1,
    PA_TILE_CTNR_POW2,
    PA_TILE_CTNR_LADDER,
    PA_TILE_CTNR_1UP,
    PA_TILE_CTRL_LEFT,
    PA_TILE_CTRL_RIGHT,
    PA_TILE_CTRL_UP,
    PA_TILE_CTRL_DOWN,
    PA_TILE_UNUSED_27,
    PA_TILE_UNUSED_28,
    PA_TILE_UNUSED_29,
    PA_TILE_INVISIBLE_BLOCK,
    PA_TILE_INVISIBLE_CONTAINER,
    PA_TILE_GRASS,
    PA_TILE_GROUND,
    PA_TILE_BRICK_BLOCK,
    PA_TILE_BLOCK__,
    PA_TILE_METAL_BLOCK,
    PA_TILE_METAL_PIPE_H,
    PA_TILE_METAL_PIPE_V,
    PA_TILE_BOUNCE_BLOCK,
    PA_TILE_DIRT_PATH,
    PA_TILE_GIRDER,
    PA_TILE_SOLID_UNUSED_42,
    PA_TILE_SOLID_UNUSED_43,
    PA_TILE_SOLID_UNUSED_44,
    PA_TILE_SOLID_UNUSED_45,
    PA_TILE_SOLID_UNUSED_46,
    PA_TILE_SOLID_UNUSED_47,
    PA_TILE_SOLID_UNUSED_48,
    PA_TILE_SOLID_UNUSED_49,
    PA_TILE_SOLID_UNUSED_50,
    PA_TILE_SOLID_UNUSED_51,
    PA_TILE_SOLID_UNUSED_52,
    PA_TILE_SOLID_UNUSED_53,
    PA_TILE_SOLID_UNUSED_54,
    PA_TILE_SOLID_UNUSED_55,
    PA_TILE_SOLID_UNUSED_56,
    PA_TILE_SOLID_UNUSED_57,
    PA_TILE_SOLID_UNUSED_58,
    PA_TILE_GOAL_100PTS,
    PA_TILE_GOAL_500PTS,
    PA_TILE_GOAL_1000PTS,
    PA_TILE_GOAL_2000PTS,
    PA_TILE_GOAL_5000PTS,
    PA_TILE_CONTAINER_1,
    PA_TILE_CONTAINER_2,
    PA_TILE_CONTAINER_3,
    PA_TILE_COIN_1,
    PA_TILE_COIN_2,
    PA_TILE_COIN_3,
    PA_TILE_LADDER,
    PA_TILE_NONSOLID_INTERACTIVE_VISIBLE_71,
    PA_TILE_NONSOLID_INTERACTIVE_VISIBLE_72,
    PA_TILE_NONSOLID_INTERACTIVE_VISIBLE_73,
    PA_TILE_NONSOLID_INTERACTIVE_VISIBLE_74,
    PA_TILE_NONSOLID_INTERACTIVE_VISIBLE_75,
    PA_TILE_NONSOLID_INTERACTIVE_VISIBLE_76,
    PA_TILE_NONSOLID_INTERACTIVE_VISIBLE_77,
    PA_TILE_NONSOLID_INTERACTIVE_VISIBLE_78,
    PA_TILE_NONSOLID_INTERACTIVE_VISIBLE_79,
    PA_TILE_BG_GOAL_ZONE,
    PA_TILE_BG_ARROW_L,
    PA_TILE_BG_ARROW_R,
    PA_TILE_BG_ARROW_U,
    PA_TILE_BG_ARROW_D,
    PA_TILE_BG_ARROW_LU,
    PA_TILE_BG_ARROW_RU,
    PA_TILE_BG_ARROW_LD,
    PA_TILE_BG_ARROW_RD,
    PA_TILE_BG_CLOUD_LD,
    PA_TILE_BG_CLOUD_M,
    PA_TILE_BG_CLOUD_RD,
    PA_TILE_BG_CLOUD_LU,
    PA_TILE_BG_CLOUD_RU,
    PA_TILE_BG_CLOUD_D,
    PA_TILE_BG_CLOUD,
    PA_TILE_BG_TALL_GRASS,
    PA_TILE_BG_MOUNTAIN_L,
    PA_TILE_BG_MOUNTAIN_U,
    PA_TILE_BG_MOUNTAIN_R,
    PA_TILE_BG_MOUNTAIN,
    PA_TILE_BG_METAL,
    PA_TILE_BG_CHAINS,
    PA_TILE_BG_WALL
} PA_TILE_Index_t;

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
    wsg_t tiles[PA_TILE_SET_SIZE];

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

    uint8_t animationFrame;
    int16_t animationTimer;
};

//==============================================================================
// Prototypes
//==============================================================================
void pa_initializeTileMap(paTilemap_t* tilemap);
void pa_drawTileMap(paTilemap_t* tilemap);
void pa_scrollTileMap(paTilemap_t* tilemap, int16_t x, int16_t y);
void pa_drawTile(paTilemap_t* tilemap, uint8_t tileId, int16_t x, int16_t y);
bool pa_loadMapFromFile(paTilemap_t* tilemap, const char* name);
bool pa_loadTiles(paTilemap_t* tilemap);
void pa_tileSpawnEntity(paTilemap_t* tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty);
uint8_t pa_getTile(paTilemap_t* tilemap, uint8_t tx, uint8_t ty);
void pa_setTile(paTilemap_t* tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId);
bool pa_isSolid(uint8_t tileId);
void pa_unlockScrolling(paTilemap_t* tilemap);
bool pa_needsTransparency(uint8_t tileId);
void pa_freeTilemap(paTilemap_t* tilemap);
void pa_generateMaze(paTilemap_t* tilemap);
bool pa_genPathContinue(paTilemap_t* tilemap, uint32_t x, uint32_t y);
void pa_genMakePath(paTilemap_t* tilemap, uint32_t x, uint32_t y);
void pa_placeEnemySpawns(paTilemap_t* tilemap);


#endif
