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


//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    wsg_t wsgs[PA_WSGS_SIZE];
    paSprite_t sprites[PA_SPRITESET_SIZE];
    wsg_t* tiles[PA_TILE_SET_SIZE];

    uint8_t globalTileAnimationFrame;
    int16_t globalTileAnimationTimer;
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
void pa_animateTiles(paWsgManager_t* self);
void pa_remapBlockTile(paWsgManager_t *self, uint16_t newBlockWsgIndex);

#endif