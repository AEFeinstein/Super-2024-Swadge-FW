#ifndef _PL_TILEMAP_H_
#define _PL_TILEMAP_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "wsg.h"
#include "platformer_typedef.h"
#include "plEntityManager.h"

//==============================================================================
// Constants
//==============================================================================
#define CLAMP(x, l, u) ((x) < l ? l : ((x) > u ? u : (x)))

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
typedef enum
{
    PL_TILEEMPTY,
    PL_TILEWARP_0,
    PL_TILEWARP_1,
    PL_TILEWARP_2,
    PL_TILEWARP_3,
    PL_TILEWARP_4,
    PL_TILEWARP_5,
    PL_TILEWARP_6,
    PL_TILEWARP_7,
    PL_TILEWARP_8,
    PL_TILEWARP_9,
    PL_TILEWARP_A,
    PL_TILEWARP_B,
    PL_TILEWARP_C,
    PL_TILEWARP_D,
    PL_TILEWARP_E,
    PL_TILEWARP_F,
    PL_TILECTNR_COIN,
    PL_TILECTNR_10COIN,
    PL_TILECTNR_POW1,
    PL_TILECTNR_POW2,
    PL_TILECTNR_LADDER,
    PL_TILECTNR_1UP,
    PL_TILECTRL_LEFT,
    PL_TILECTRL_RIGHT,
    PL_TILECTRL_UP,
    PL_TILECTRL_DOWN,
    PL_TILEUNUSED_27,
    PL_TILEUNUSED_28,
    PL_TILEUNUSED_29,
    PL_TILEINVISIBLE_BLOCK,
    PL_TILEINVISIBLE_CONTAINER,
    PL_TILEGRASS,
    PL_TILEGROUND,
    PL_TILEBRICK_BLOCK,
    PL_TILEBLOCK,
    PL_TILEMETAL_BLOCK,
    PL_TILEMETAL_PIPE_H,
    PL_TILEMETAL_PIPE_V,
    PL_TILEBOUNCE_BLOCK,
    PL_TILEDIRT_PATH,
    PL_TILEGIRDER,
    PL_TILESOLID_UNUSED_42,
    PL_TILESOLID_UNUSED_43,
    PL_TILESOLID_UNUSED_44,
    PL_TILESOLID_UNUSED_45,
    PL_TILESOLID_UNUSED_46,
    PL_TILESOLID_UNUSED_47,
    PL_TILESOLID_UNUSED_48,
    PL_TILESOLID_UNUSED_49,
    PL_TILESOLID_UNUSED_50,
    PL_TILESOLID_UNUSED_51,
    PL_TILESOLID_UNUSED_52,
    PL_TILESOLID_UNUSED_53,
    PL_TILESOLID_UNUSED_54,
    PL_TILESOLID_UNUSED_55,
    PL_TILESOLID_UNUSED_56,
    PL_TILESOLID_UNUSED_57,
    PL_TILESOLID_UNUSED_58,
    PL_TILEGOAL_100PTS,
    PL_TILEGOAL_500PTS,
    PL_TILEGOAL_1000PTS,
    PL_TILEGOAL_2000PTS,
    PL_TILEGOAL_5000PTS,
    PL_TILECONTAINER_1,
    PL_TILECONTAINER_2,
    PL_TILECONTAINER_3,
    PL_TILECOIN_1,
    PL_TILECOIN_2,
    PL_TILECOIN_3,
    PL_TILELADDER,
    PL_TILENONSOLID_INTERACTIVE_VISIBLE_71,
    PL_TILENONSOLID_INTERACTIVE_VISIBLE_72,
    PL_TILENONSOLID_INTERACTIVE_VISIBLE_73,
    PL_TILENONSOLID_INTERACTIVE_VISIBLE_74,
    PL_TILENONSOLID_INTERACTIVE_VISIBLE_75,
    PL_TILENONSOLID_INTERACTIVE_VISIBLE_76,
    PL_TILENONSOLID_INTERACTIVE_VISIBLE_77,
    PL_TILENONSOLID_INTERACTIVE_VISIBLE_78,
    PL_TILENONSOLID_INTERACTIVE_VISIBLE_79,
    PL_TILEBG_GOAL_ZONE,
    PL_TILEBG_ARROW_L,
    PL_TILEBG_ARROW_R,
    PL_TILEBG_ARROW_U,
    PL_TILEBG_ARROW_D,
    PL_TILEBG_ARROW_LU,
    PL_TILEBG_ARROW_RU,
    PL_TILEBG_ARROW_LD,
    PL_TILEBG_ARROW_RD,
    PL_TILEBG_CLOUD_LD,
    PL_TILEBG_CLOUD_M,
    PL_TILEBG_CLOUD_RD,
    PL_TILEBG_CLOUD_LU,
    PL_TILEBG_CLOUD_RU,
    PL_TILEBG_CLOUD_D,
    PL_TILEBG_CLOUD,
    PL_TILEBG_TALL_GRASS,
    PL_TILEBG_MOUNTAIN_L,
    PL_TILEBG_MOUNTAIN_U,
    PL_TILEBG_MOUNTAIN_R,
    PL_TILEBG_MOUNTAIN,
    PL_TILEBG_METAL,
    PL_TILEBG_CHAINS,
    PL_TILEBG_WALL
} pl_tileIndex_t;

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
    wsg_t tiles[PL_TILESET_SIZE];

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
void pl_initializeTileMap(plTilemap_t* tilemap);
void pl_drawTileMap(plTilemap_t* tilemap);
void pl_scrollTileMap(plTilemap_t* tilemap, int16_t x, int16_t y);
void pl_drawTile(plTilemap_t* tilemap, uint8_t tileId, int16_t x, int16_t y);
bool pl_loadMapFromFile(plTilemap_t* tilemap, const char* name);
bool pl_loadTiles(plTilemap_t* tilemap);
void pl_tileSpawnEntity(plTilemap_t* tilemap, uint8_t objectIndex, uint8_t tx, uint8_t ty);
uint8_t pl_getTile(plTilemap_t* tilemap, uint8_t tx, uint8_t ty);
void pl_setTile(plTilemap_t* tilemap, uint8_t tx, uint8_t ty, uint8_t newTileId);
bool pl_isSolid(uint8_t tileId);
void pl_unlockScrolling(plTilemap_t* tilemap);
bool pl_needsTransparency(uint8_t tileId);
void pl_freeTilemap(plTilemap_t* tilemap);

#endif
