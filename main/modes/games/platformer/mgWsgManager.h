#ifndef _MG_WSGMANAGER_H_
#define _MG_WSGMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include "wsg.h"
#include "mgSprite.h"
#include "mega_pulse_ex_typedef.h"

//==============================================================================
// Constants
//==============================================================================
#define MG_WSGS_SIZE      154
#define MG_SPRITESET_SIZE 64
#define MG_TILE_SET_SIZE  72

//==============================================================================
// Enums
//==============================================================================

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    wsg_t wsgs[MG_WSGS_SIZE];
    mgSprite_t sprites[MG_SPRITESET_SIZE];
    wsg_t* tiles[MG_TILE_SET_SIZE];

    uint8_t globalTileAnimationFrame;
    int16_t globalTileAnimationTimer;

    int8_t wsgSetIndex;
} mgWsgManager_t;

//==============================================================================
// Function Definitions
//==============================================================================

void mg_initializeWsgManager(mgWsgManager_t* self);
void mg_freeWsgManager(mgWsgManager_t* self);

void mg_loadWsgs(mgWsgManager_t* self);
void mg_initializeSprites(mgWsgManager_t* self);
void mg_initializeTiles(mgWsgManager_t* tiles);

void mg_remapWsgToSprite(mgWsgManager_t* self, uint16_t spriteIndex, uint16_t wsgIndex);
void mg_remapWsgToTile(mgWsgManager_t* self, uint16_t tileIndex, uint16_t wsgIndex);

void mg_remapPlayerCharacter(mgWsgManager_t* self, uint16_t newBaseIndex);

void mg_remapPlayerShootWsg(mgWsgManager_t* self);
void mg_remapPlayerNotShootWsg(mgWsgManager_t* self);

void mg_animateTiles(mgWsgManager_t* self);
void mg_remapBlockTile(mgWsgManager_t* self, uint16_t newBlockWsgIndex);
void mg_loadWsgSet(mgWsgManager_t* self, mgWsgSetIndex_t index);

#endif