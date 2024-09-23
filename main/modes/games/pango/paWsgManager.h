#ifndef _PA_WSGMANAGER_H_
#define _PA_WSGMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include "wsg.h"
#include "paSprite.h"
#include "pango_typedef.h"

//==============================================================================
// Constants
//==============================================================================
#define PA_WSGS_SIZE      93
#define PA_SPRITESET_SIZE 32
#define PA_TILE_SET_SIZE  15

//==============================================================================
// Enums
//==============================================================================
typedef enum
{
    PA_WSG_PANGO_SOUTH,
    PA_WSG_PANGO_WALK_SOUTH,
    PA_WSG_PANGO_NORTH,
    PA_WSG_PANGO_WALK_NORTH,
    PA_WSG_PANGO_SIDE,
    PA_WSG_PANGO_WALK_SIDE_1,
    PA_WSG_PANGO_WALK_SIDE_2,
    PA_WSG_PANGO_PUSH_SOUTH_1,
    PA_WSG_PANGO_PUSH_SOUTH_2,
    PA_WSG_PANGO_PUSH_NORTH_1,
    PA_WSG_PANGO_PUSH_NORTH_2,
    PA_WSG_PANGO_PUSH_SIDE_1,
    PA_WSG_PANGO_PUSH_SIDE_2,
    PA_WSG_PANGO_HURT,
    PA_WSG_PANGO_WIN,
    PA_WSG_PANGO_ICON,
    PA_WSG_PO_SOUTH,
    PA_WSG_PO_WALK_SOUTH,
    PA_WSG_PO_NORTH,
    PA_WSG_PO_WALK_NORTH,
    PA_WSG_PO_SIDE,
    PA_WSG_PO_WALK_SIDE_1,
    PA_WSG_PO_WALK_SIDE_2,
    PA_WSG_PO_PUSH_SOUTH_1,
    PA_WSG_PO_PUSH_SOUTH_2,
    PA_WSG_PO_PUSH_NORTH_1,
    PA_WSG_PO_PUSH_NORTH_2,
    PA_WSG_PO_PUSH_SIDE_1,
    PA_WSG_PO_PUSH_SIDE_2,
    PA_WSG_PO_HURT,
    PA_WSG_PO_WIN,
    PA_WSG_PO_ICON,
    PA_WSG_PIXEL_SOUTH,
    PA_WSG_PIXEL_WALK_SOUTH,
    PA_WSG_PIXEL_NORTH,
    PA_WSG_PIXEL_WALK_NORTH,
    PA_WSG_PIXEL_SIDE,
    PA_WSG_PIXEL_WALK_SIDE_1,
    PA_WSG_PIXEL_WALK_SIDE_2,
    PA_WSG_PIXEL_PUSH_SOUTH_1,
    PA_WSG_PIXEL_PUSH_SOUTH_2,
    PA_WSG_PIXEL_PUSH_NORTH_1,
    PA_WSG_PIXEL_PUSH_NORTH_2,
    PA_WSG_PIXEL_PUSH_SIDE_1,
    PA_WSG_PIXEL_PUSH_SIDE_2,
    PA_WSG_PIXEL_HURT,
    PA_WSG_PIXEL_WIN,
    PA_WSG_PIXEL_ICON,
    PA_WSG_GIRL_SOUTH,
    PA_WSG_GIRL_WALK_SOUTH,
    PA_WSG_GIRL_NORTH,
    PA_WSG_GIRL_WALK_NORTH,
    PA_WSG_GIRL_SIDE,
    PA_WSG_GIRL_WALK_SIDE_1,
    PA_WSG_GIRL_WALK_SIDE_2,
    PA_WSG_GIRL_PUSH_SOUTH_1,
    PA_WSG_GIRL_PUSH_SOUTH_2,
    PA_WSG_GIRL_PUSH_NORTH_1,
    PA_WSG_GIRL_PUSH_NORTH_2,
    PA_WSG_GIRL_PUSH_SIDE_1,
    PA_WSG_GIRL_PUSH_SIDE_2,
    PA_WSG_GIRL_HURT,
    PA_WSG_GIRL_WIN,
    PA_WSG_GIRL_ICON,
    // PA_WSG_BLOCK,
    // PA_WSG_BONUS_BLOCK,
    PA_WSG_ENEMY_SOUTH,
    PA_WSG_ENEMY_NORTH,
    PA_WSG_ENEMY_SIDE_1,
    PA_WSG_ENEMY_SIDE_2,
    PA_WSG_ENEMY_DRILL_SOUTH,
    PA_WSG_ENEMY_DRILL_NORTH,
    PA_WSG_ENEMY_DRILL_SIDE_1,
    PA_WSG_ENEMY_DRILL_SIDE_2,
    PA_WSG_ENEMY_STUN,
    PA_WSG_BREAK_BLOCK,
    PA_WSG_BREAK_BLOCK_1,
    PA_WSG_BREAK_BLOCK_2,
    PA_WSG_BREAK_BLOCK_3,
    PA_WSG_BLOCK_FRAGMENT,
    PA_WSG_WALL_0,
    PA_WSG_WALL_1,
    PA_WSG_WALL_2,
    PA_WSG_WALL_3,
    PA_WSG_WALL_4,
    PA_WSG_WALL_5,
    PA_WSG_WALL_6,
    PA_WSG_WALL_7,
    PA_WSG_BLOCK,
    PA_WSG_SPAWN_BLOCK_0,
    PA_WSG_SPAWN_BLOCK_1,
    PA_WSG_SPAWN_BLOCK_2,
    PA_WSG_BONUS_BLOCK_0,
    PA_WSG_BONUS_BLOCK_1,
    PA_WSG_BONUS_BLOCK_2
} paWsgIndex_t;

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    wsg_t wsgs[PA_WSGS_SIZE];
    paSprite_t sprites[PA_SPRITESET_SIZE];
    wsg_t* tiles[PA_TILE_SET_SIZE];
} paWsgManager_t;

//==============================================================================
// Function Definitions
//==============================================================================
void pa_initializeWsgManager(paWsgManager_t* self);
void pa_freeWsgManager(paWsgManager_t* self);

void pa_loadWsgs(paWsgManager_t* self);
void pa_initializeSprites(paWsgManager_t* self);
void pa_initializeTiles(paWsgManager_t* tiles);

void pa_remapWsgToSprite(paWsgManager_t* self, uint16_t spriteIndex, uint16_t wsgIndex);
void pa_remapWsgToTile(paWsgManager_t* self, uint16_t tileIndex, uint16_t wsgIndex);

void pa_remapPlayerCharacter(paWsgManager_t* self, uint16_t newBaseIndex);

#endif