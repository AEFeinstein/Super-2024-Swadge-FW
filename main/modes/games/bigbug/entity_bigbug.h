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

typedef struct
{
    vec_t vel;         // velocity
    vec_t accel;       // acceleration
    vec_t previousPos; // position from the previous frame
    vec_t yaw;         //.x is the yaw, .y is the change in yaw over time. Gravitates toward left or right.
    // uint8_t numHarpoons;//number of harpoons

    // touchpad stuff
    bool touching;
    bool fire; // becomes true for a frame upon touchpad release "event"
    int32_t phi;
    int32_t r;
    int32_t intensity;
} bb_garbotnikData;

typedef struct
{
    vec_t vel;
} bb_projectileData;

typedef struct
{
    bb_entity_t* flame; // tracks the flame to update position like a child object
    int32_t yVel;
} bb_rocketData_t;

typedef struct
{
    int32_t yVel;
} bb_heavyFallingData_t;

typedef void (*bb_updateFunction_t)(bb_entity_t* self);
typedef void (*bb_drawFunction_t)(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
typedef void (*bb_collisionHandler_t)(bb_entity_t* self, bb_entity_t* other);
typedef bool (*bb_tileCollisionHandler_t)(bb_entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
typedef void (*bb_fallOffTileHandler_t)(bb_entity_t* self);
typedef void (*bb_overlapTileHandler_t)(bb_entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

struct bb_entity_t
{
    bool active;

    void* data;
    bb_updateFunction_t updateFunction; // Only set for entities that need update logic
    bb_drawFunction_t drawFunction;     // Only set for custom entities such as Garbotnik

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

    int16_t halfWidth;  // Distance from the origin to the side edge (for AABB physics)
    int16_t halfHeight; // Distance from the origin to the top edge (for AABB physics)
    int32_t cSquared;   // Squared distance from the sprite origin to the corner of the AABB hitbox. Used for collision
                        // optimization.

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
void bb_updateRocketLanding(bb_entity_t* self);
void bb_updateHeavyFallingInit(bb_entity_t* self);
void bb_updateHeavyFalling(bb_entity_t* self);
void bb_updateGarbotnikDeploy(bb_entity_t* self);
void bb_updateFlame(bb_entity_t* self);
void bb_updateGarbotnikFlying(bb_entity_t* self);
void bb_updateHarpoon(bb_entity_t* self);
void bb_drawGarbotnikFlying(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawHarpoon(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);

#endif