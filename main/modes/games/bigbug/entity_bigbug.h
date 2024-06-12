#ifndef _ENTITY_BIGBUG_H_
#define _ENTITY_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>
#include <stdbool.h>
#include "gameData_bigbug.h"
#include "soundManager_bigbug.h"
#include "sprite_bigbug.h"

#include "linked_list.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    ENTITY_BUMP,    //A particle effect where you bump things
    ENTITY_WILE,    //A metal ball that is thrown
    ENTITY_HARPOON, //A harpoon you throw
    ENTITY_FLY,     //A bug that flies
    ENTITY_ANT,     //A bug that crawls on the midground tile field
    ENTITY_BEETLE,  //A bug that walks on the the foreground tile field
    ENTITY_UNUSED_6,
    ENTITY_UNUSED_7,
    ENTITY_UNUSED_8,
    ENTITY_UNUSED_9,
    ENTITY_UNUSED_10,
    ENTITY_UNUSED_11,
    ENTITY_UNUSED_12,
    ENTITY_UNUSED_13,
    ENTITY_UNUSED_14,
    ENTITY_UNUSED_15,
    ENTITY_UNUSED_16,
    ENTITY_UNUSED_17,
    ENTITY_UNUSED_18,
    ENTITY_UNUSED_19,
    ENTITY_UNUSED_20,
    ENTITY_UNUSED_21
} bb_entityIndex_t;

typedef enum{
    ONESHOT_ANIMATION,
    PHYSICS_SPRITE,
    PHYSICS_ANIMATION
} bb_entityType_t;

//==============================================================================
// Structs
//==============================================================================

typedef void (*bb_updateFunction_t)(struct bb_entity_t* self);
typedef void (*bb_collisionHandler_t)(struct bb_entity_t* self, struct bb_entity_t* other);
typedef bool (*bb_tileCollisionHandler_t)(struct bb_entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty,
                                       uint8_t direction);
typedef void (*bb_fallOffTileHandler_t)(struct bb_entity_t* self);
typedef void (*bb_overlapTileHandler_t)(struct bb_entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);


struct bb_entity_t
{
    bool active;

    uint8_t type;
    bb_updateFunction_t updateFunction;

    uint16_t x;
    uint16_t y;

    int16_t xspeed;
    int16_t yspeed;

    bb_sprite_t sprite;
    node_t* currentFrame;//->value is a wsg_t
    bool spriteFlipHorizontal;
    bool spriteFlipVertical;
    int16_t spriteRotateAngle;

    uint8_t animationTimer;
    uint8_t animationFPS;

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

#endif