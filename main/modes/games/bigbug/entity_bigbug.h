#ifndef _ENTITY_BIGBUG_H_
#define _ENTITY_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>
#include <stdbool.h>
#include "gameData_bigbug.h"
#include "entityManager_bigbug.h"
#include "soundManager_bigbug.h"
#include "sprite_bigbug.h"

#include "linked_list.h"
#include "vector2d.h"

//==============================================================================
// Enums
//==============================================================================



//==============================================================================
// Structs
//==============================================================================

typedef struct{
    vec_t vel;   //velocity
    vec_t accel; //acceleration
    vec_t previousPos; //position from the previous frame
    vec_t yaw;   //.x is the yaw, .y is the change in yaw over time. Gravitates toward left or right.
} bb_garbotnikData;

typedef void (*bb_updateFunction_t)(struct bb_entity_t* self);
typedef void (*bb_drawFunction_t)(struct bb_entity_t* self, struct bb_gameData_t* gameData);
typedef void (*bb_collisionHandler_t)(struct bb_entity_t* self, struct bb_entity_t* other);
typedef bool (*bb_tileCollisionHandler_t)(struct bb_entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty,
                                          uint8_t direction);
typedef void (*bb_fallOffTileHandler_t)(struct bb_entity_t* self);
typedef void (*bb_overlapTileHandler_t)(struct bb_entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

struct bb_entity_t
{
    bool active;

    void* data;
    bb_updateFunction_t updateFunction;
    bb_drawFunction_t drawFunction;

    vec_t pos;
    
    bb_animationType_t type;
    bb_spriteDef_t spriteIndex;
    bool paused;
    
    uint16_t animationTimer;
    uint8_t gameFramesPerAnimationFrame;
    uint8_t currentAnimationFrame;

    bb_gameData_t* gameData;
    bb_soundManager_t* soundManager;
    bb_entityManager_t* entityManager;

    bb_collisionHandler_t collisionHandler;
    bb_tileCollisionHandler_t tileCollisionHandler;
    bb_overlapTileHandler_t overlapTileHandler;
};

//==============================================================================
// Prototypes
//==============================================================================
void bb_initializeEntity(bb_entity_t* self, bb_entityManager_t* entityManager, bb_gameData_t* gameData,
                         bb_soundManager_t* soundManager);

void bb_destroyEntity(bb_entity_t* self, bool respawn);
void bb_updateGarbotnikFlying(bb_entity_t* self);

#endif