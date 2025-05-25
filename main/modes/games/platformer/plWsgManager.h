#ifndef _PL_WSGMANAGER_H_
#define _PL_WSGMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include "wsg.h"
#include "plSprite.h"
#include "platformer_typedef.h"

//==============================================================================
// Constants
//==============================================================================
#define PL_WSGS_SIZE      105
#define PL_SPRITESET_SIZE 62
#define PL_TILE_SET_SIZE  72

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    wsg_t wsgs[PL_WSGS_SIZE];
    plSprite_t sprites[PL_SPRITESET_SIZE];
    wsg_t* tiles[PL_TILE_SET_SIZE];

    uint8_t globalTileAnimationFrame;
    int16_t globalTileAnimationTimer;
} plWsgManager_t;

//==============================================================================
// Function Definitions
//==============================================================================

void pl_initializeWsgManager(plWsgManager_t* self);
void pl_freeWsgManager(plWsgManager_t* self);

void pl_loadWsgs(plWsgManager_t* self);
void pl_initializeSprites(plWsgManager_t* self);
void pl_initializeTiles(plWsgManager_t* tiles);

void pl_remapWsgToSprite(plWsgManager_t* self, uint16_t spriteIndex, uint16_t wsgIndex);
void pl_remapWsgToTile(plWsgManager_t* self, uint16_t tileIndex, uint16_t wsgIndex);

void pl_remapPlayerCharacter(plWsgManager_t* self, uint16_t newBaseIndex);
void pl_animateTiles(plWsgManager_t* self);
void pl_remapBlockTile(plWsgManager_t* self, uint16_t newBlockWsgIndex);

#endif