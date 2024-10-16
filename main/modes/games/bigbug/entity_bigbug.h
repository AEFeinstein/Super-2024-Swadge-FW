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
    vec_t vel;           // velocity
    vec_t accel;         // acceleration
    vec_t previousPos;   // position from the previous frame
    vec_t yaw;           //.x is the yaw, .y is the change in yaw over time. Gravitates toward left or right.
    uint8_t numHarpoons; // number of harpoons
    int32_t fuel;        // garbotnik's remaining fuel. Like a level timer that can be influenced.

    // touchpad stuff
    bool touching;
    bool fire; // becomes true for a frame upon touchpad release "event"
    int32_t phi;
    int32_t r;
    int32_t intensity;
} bb_garbotnikData_t;

typedef struct
{
    bb_entity_t* parent;
    vec_t offset;
    uint16_t lifetime;
    vecFl_t floatVel;
} bb_stuckHarpoonData_t;

typedef struct
{
    int8_t health;
} bb_bugData_t;

typedef struct
{
    vec_t vel;
    uint16_t lifetime;
} bb_projectileData_t;

typedef struct
{
    bb_entity_t* flame; // tracks the flame to update position like a child object
    int32_t yVel;
} bb_rocketData_t;

typedef struct
{
    int32_t yVel;
} bb_heavyFallingData_t;

typedef struct
{
    uint8_t brightness; // stores brightness so that it may be stored so it may be calculated once then used to
                        // stimulate the egg and also to draw.
    bb_entity_t* egg;   // tracks the egg to stimulate it.
} bb_eggLeavesData_t;

typedef struct
{
    uint16_t stimulation; // once it reaches 600, it turns into a bug.
} bb_eggData_t;

typedef void (*bb_updateFunction_t)(bb_entity_t* self);
typedef void (*bb_updateFarFunction_t)(bb_entity_t* self);
typedef void (*bb_drawFunction_t)(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
typedef void (*bb_collisionHandler_t)(bb_entity_t* self, bb_entity_t* other);
typedef struct
{
    list_t* checkOthers; // A list of bb_spriteDef_t's to check collision against. i.e. all bug spriteDef indices for
                         // the harpoon.
    bb_collisionHandler_t function; // Triggers on collision enter with any of the checkOthers.
} bb_collision_t;
typedef bool (*bb_tileCollisionHandler_t)(bb_entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
typedef void (*bb_fallOffTileHandler_t)(bb_entity_t* self);
typedef void (*bb_overlapTileHandler_t)(bb_entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

struct bb_entity_t
{
    bool active;
    bool cacheable;

    void* data;
    bb_updateFunction_t updateFunction;       // Only set for entities that need update logic
    bb_updateFarFunction_t updateFarFunction; // Only set for execution when the entity is far from the camera center
    bb_drawFunction_t drawFunction;           // Only set for entities such as Garbotnik that need custom drawing logic
    list_t* collisions; // It's a list of bb_collision_t which each get processed by the entity manager.

    vec_t pos;

    bb_animationType_t type;
    bb_spriteDef_t spriteIndex;
    bool paused;
    bool hasLighting; // True if it has 6 lighting versions per frame

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

    bb_tileCollisionHandler_t tileCollisionHandler;
    bb_overlapTileHandler_t overlapTileHandler;
};

//==============================================================================
// Prototypes
//==============================================================================
void bb_initializeEntity(bb_entity_t* self, bb_entityManager_t* entityManager, bb_gameData_t* gameData,
                         bb_soundManager_t* soundManager);

void bb_destroyEntity(bb_entity_t* self, bool caching);
void bb_updateRocketLanding(bb_entity_t* self);
void bb_updateHeavyFallingInit(bb_entity_t* self);
void bb_updateHeavyFalling(bb_entity_t* self);
void bb_updateGarbotnikDeploy(bb_entity_t* self);
void bb_updateGarbotnikFlying(bb_entity_t* self);
void bb_updateHarpoon(bb_entity_t* self);
void bb_updateStuckHarpoon(bb_entity_t* self);
void bb_updateEggLeaves(bb_entity_t* self);
void bb_updateFarEggleaves(bb_entity_t* self);
void bb_updateBug(bb_entity_t* self);

void bb_drawGarbotnikFlying(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawHarpoon(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawEggLeaves(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);
void bb_drawEgg(bb_entityManager_t* entityManager, rectangle_t* camera, bb_entity_t* self);

void bb_onCollisionHarpoon(bb_entity_t* self, bb_entity_t* other);

#endif