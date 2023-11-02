#ifndef _TILEMAP_H_
#define _TILEMAP_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "wsg.h"
#include "breakout_typedef.h"
#include "entityManager.h"

//==============================================================================
// Constants
//==============================================================================
#define CLAMP(x, l, u) ((x) < l ? l : ((x) > u ? u : (x)))

#define TILEMAP_DISPLAY_WIDTH_PIXELS 280  // The screen size
#define TILEMAP_DISPLAY_HEIGHT_PIXELS 240 // The screen size
#define TILEMAP_DISPLAY_WIDTH_TILES 36    // The screen size in tiles + 1
#define TILEMAP_DISPLAY_HEIGHT_TILES 31   // The screen size in tiles + 1

#define TILE_SIZE 8
#define TILE_SIZE_IN_POWERS_OF_2 3

#define TILESET_SIZE 128

//==============================================================================
// Enums
//==============================================================================
typedef enum {
    TILE_EMPTY,
    TILE_BOUNDARY_1,
    TILE_BOUNDARY_2,
    TILE_BOUNDARY_3,
    TILE_UNUSED_4,
    TILE_UNUSED_5,
    TILE_UNUSED_6,
    TILE_UNUSED_7,
    TILE_INVISIBLE_BLOCK,
    TILE_UNUSED_9,
    TILE_UNUSED_A,
    TILE_UNUSED_B,
    TILE_UNUSED_C,
    TILE_UNUSED_D,
    TILE_UNUSED_E,
    TILE_UNUSED_F,
    TILE_BLOCK_1x1_RED,
    TILE_BLOCK_1x1_ORANGE,
    TILE_BLOCK_1x1_YELLOW,
    TILE_BLOCK_1x1_GREEN,
    TILE_BLOCK_1x1_CYAN,
    TILE_BLOCK_1x1_BLUE,
    TILE_BLOCK_1x1_PURPLE,
    TILE_BLOCK_1x1_MAGENTA,
    TILE_BLOCK_1x1_WHITE,
    TILE_BLOCK_1x1_TAN,
    TILE_BLOCK_1x1_BROWN,
    TILE_BLOCK_1x1_BLACK,
    TILE_UNUSED_28,
    TILE_UNUSED_29,
    TILE_UNUSED_30,
    TILE_UNUSED_31,
    TILE_BLOCK_2x1_RED_L,
    TILE_BLOCK_2x1_RED_R,
    TILE_BLOCK_2x1_ORANGE_L,
    TILE_BLOCK_2x1_ORANGE_R,
    TILE_BLOCK_2x1_YELLOW_L,
    TILE_BLOCK_2x1_YELLOW_R,
    TILE_BLOCK_2x1_GREEN_L,
    TILE_BLOCK_2x1_GREEN_R,
    TILE_BLOCK_2x1_CYAN_L,
    TILE_BLOCK_2x1_CYAN_R,
    TILE_BLOCK_2x1_BLUE_L,
    TILE_BLOCK_2x1_BLUE_R,
    TILE_BLOCK_2x1_PURPLE_L,
    TILE_BLOCK_2x1_PURPLE_R,
    TILE_BLOCK_2x1_MAGENTA_L,
    TILE_BLOCK_2x1_MAGENTA_R,
    TILE_BLOCK_2x1_WHITE_L,
    TILE_BLOCK_2x1_WHITE_R,
    TILE_BLOCK_2x1_TAN_L,
    TILE_BLOCK_2x1_TAN_R,
    TILE_BLOCK_2x1_BROWN_L,
    TILE_BLOCK_2x1_BROWN_R,
    TILE_BLOCK_2x1_BLACK_L,
    TILE_BLOCK_2x1_BLACK_R,
    TILE_SOLID_UNUSED_56,
    TILE_SOLID_UNUSED_57,
    TILE_SOLID_UNUSED_58,
    TILE_SOLID_UNUSED_59,
    TILE_SOLID_UNUSED_60,
    TILE_SOLID_UNUSED_61,
    TILE_SOLID_UNUSED_62,
    TILE_SOLID_UNUSED_63,
    TILE_BLOCK_2x2_RED_UL,
    TILE_BLOCK_2x2_RED_UR,
    TILE_BLOCK_2x2_ORANGE_UL,
    TILE_BLOCK_2x2_ORANGE_UR,
    TILE_BLOCK_2x2_YELLOW_UL,
    TILE_BLOCK_2x2_YELLOW_UR,
    TILE_BLOCK_2x2_GREEN_UL,
    TILE_BLOCK_2x2_GREEN_UR,
    TILE_BLOCK_2x2_CYAN_UL,
    TILE_BLOCK_2x2_CYAN_UR,
    TILE_BLOCK_2x2_BLUE_UL,
    TILE_BLOCK_2x2_BLUE_UR,
    TILE_BLOCK_2x2_PURPLE_UL,
    TILE_BLOCK_2x2_PURPLE_UR,
    TILE_BLOCK_2x2_MAGENTA_UL,
    TILE_BLOCK_2x2_MAGENTA_UR,
    TILE_BLOCK_2x2_RED_DL,
    TILE_BLOCK_2x2_RED_DR,
    TILE_BLOCK_2x2_ORANGE_DL,
    TILE_BLOCK_2x2_ORANGE_DR,
    TILE_BLOCK_2x2_YELLOW_DL,
    TILE_BLOCK_2x2_YELLOW_DR,
    TILE_BLOCK_2x2_GREEN_DL,
    TILE_BLOCK_2x2_GREEN_DR,
    TILE_BLOCK_2x2_CYAN_DL,
    TILE_BLOCK_2x2_CYAN_DR,
    TILE_BLOCK_2x2_BLUE_DL,
    TILE_BLOCK_2x2_BLUE_DR,
    TILE_BLOCK_2x2_PURPLE_DL,
    TILE_BLOCK_2x2_PURPLE_DR,
    TILE_BLOCK_2x2_MAGENTA_DL,
    TILE_BLOCK_2x2_MAGENTA_DR,
    TILE_BLOCK_2x2_WHITE_UL,
    TILE_BLOCK_2x2_WHITE_UR,
    TILE_BLOCK_2x2_TAN_UL,
    TILE_BLOCK_2x2_TAN_UR,
    TILE_BLOCK_2x2_BROWN_UL,
    TILE_BLOCK_2x2_BROWN_UR,
    TILE_BLOCK_2x2_BLACK_UL,
    TILE_BLOCK_2x2_BLACK_UR,
    TILE_UNUSED_104,
    TILE_UNUSED_105,
    TILE_UNUSED_106,
    TILE_UNUSED_107,
    TILE_UNUSED_108,
    TILE_UNUSED_109,
    TILE_UNUSED_110,
    TILE_UNUSED_111,
    TILE_BLOCK_2x2_WHITE_DL,
    TILE_BLOCK_2x2_WHITE_DR,
    TILE_BLOCK_2x2_TAN_DL,
    TILE_BLOCK_2x2_TAN_DR,
    TILE_BLOCK_2x2_BROWN_DL,
    TILE_BLOCK_2x2_BROWN_DR,
    TILE_BLOCK_2x2_BLACK_DL,
    TILE_BLOCK_2x2_BLACK_DR,
    TILE_UNUSED_120,
    TILE_UNUSED_121,
    TILE_UNUSED_122,
    TILE_UNUSED_123,
    TILE_UNUSED_124,
    TILE_UNUSED_125,
    TILE_UNUSED_126,
    TILE_UNUSED_127
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
    
    uint16_t totalTargetBlocks;

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
bool isBlock(uint8_t tileId);
void unlockScrolling(tilemap_t *tilemap);
bool needsTransparency(uint8_t tileId);
void freeTilemap(tilemap_t *tilemap);
void forceTileSpawnEntitiesWithinView(tilemap_t *tilemap);

#endif
