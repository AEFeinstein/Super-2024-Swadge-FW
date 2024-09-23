#ifndef _PA_ENTITY_H_
#define _PA_ENTITY_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "pango_typedef.h"
#include "paTilemap.h"
#include "paGameData.h"
#include "paSoundManager.h"
#include "shapes.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    ENTITY_PLAYER,
    PA_ENTITY_CRABDOZER,
    PA_ENTITY_BREAK_BLOCK,
    PA_ENTITY_BLOCK_FRAGMENT,
    ENTITY_HIT_BLOCK,
    ENTITY_DEAD
} paEntityIndex_t;

typedef enum
{
    PA_DIRECTION_NONE,
    PA_DIRECTION_NORTH,
    PA_DIRECTION_SOUTH,
    PA_DIRECTION_NULL_3,
    PA_DIRECTION_WEST,
    PA_DIRECTION_NORTHWEST,
    PA_DIRECTION_SOUTHWEST,
    PA_DIRECTION_NULL_7,
    PA_DIRECTION_EAST,
    PA_DIRECTION_NORTHEAST,
    PA_DIRECTION_SOUTHEAST
} paCompassDirection_t;

typedef enum
{
    PA_EN_ST_SPAWNING,
    PA_EN_ST_STUN,
    PA_EN_ST_NORMAL,
    PA_EN_ST_AGGRESSIVE,
    PA_EN_ST_RUNAWAY,
    PA_EN_ST_BREAK_BLOCK,
} paEnemyState_t;

typedef enum
{
    PA_PL_ST_NORMAL,
    PA_PL_ST_PUSHING
} paPlayerState_t;

//==============================================================================
// Structs
//==============================================================================

typedef void (*pa_updateFunction_t)(struct paEntity_t* self);
typedef void (*pa_collisionHandler_t)(struct paEntity_t* self, struct paEntity_t* other);
typedef bool (*pa_tileCollisionHandler_t)(struct paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty,
                                           uint8_t direction);
typedef void (*pa_fallOffTileHandler_t)(struct paEntity_t* self);
typedef void (*pa_overlapTileHandler_t)(struct paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

struct paEntity_t
{
    bool active;
    // bool important;

    uint8_t type;
    pa_updateFunction_t updateFunction;

    uint16_t x;
    uint16_t y;

    int16_t xspeed;
    int16_t yspeed;

    int16_t xMaxSpeed;
    int16_t yMaxSpeed;

    int16_t xDamping;
    int16_t yDamping;

    bool gravityEnabled;
    int16_t gravity;
    bool falling;

    uint16_t facingDirection;

    uint8_t spriteIndex;
    bool spriteFlipHorizontal;
    bool spriteFlipVertical;
    uint8_t animationTimer;

    paTilemap_t* tilemap;
    paGameData_t* gameData;
    paSoundManager_t* soundManager;

    uint8_t homeTileX;
    uint8_t homeTileY;

    bool visible;
    uint8_t hp;
    int8_t invincibilityFrames;
    uint16_t scoreValue;

    uint8_t targetTileX;
    uint8_t targetTileY;
    uint16_t state;
    bool stateFlag;
    int16_t stateTimer;
    int16_t tempStateTimer;
    int16_t baseSpeed;

    // paEntity_t *entities;
    paEntityManager_t* entityManager;

    pa_collisionHandler_t collisionHandler;
    pa_tileCollisionHandler_t tileCollisionHandler;
    pa_overlapTileHandler_t overlapTileHandler;
};

//==============================================================================
// Prototypes
//==============================================================================
void pa_initializeEntity(paEntity_t* self, paEntityManager_t* entityManager, paTilemap_t* tilemap,
                         paGameData_t* gameData, paSoundManager_t* soundManager);

void pa_updatePlayer(paEntity_t* self);
void updateCrabdozer(paEntity_t* self);
void pa_enemyChangeDirection(paEntity_t* self, uint16_t newDirection, int16_t speed);
void pa_enemyBreakBlock(paEntity_t* self, uint16_t newDirection, int16_t speed, uint8_t tx, uint8_t ty);
void pa_animateEnemy(paEntity_t* self);
void updateHitBlock(paEntity_t* self);

void pa_moveEntityWithTileCollisions(paEntity_t* self);
void defaultFallOffTileHandler(paEntity_t* self);

void despawnWhenOffscreen(paEntity_t* self);

void pa_destroyEntity(paEntity_t* self, bool respawn);

void applyDamping(paEntity_t* self);

void applyGravity(paEntity_t* self);

void animatePlayer(paEntity_t* self);

void pa_detectEntityCollisions(paEntity_t* self);

void pa_playerCollisionHandler(paEntity_t* self, paEntity_t* other);
void pa_enemyCollisionHandler(paEntity_t* self, paEntity_t* other);
void pa_dummyCollisionHandler(paEntity_t* self, paEntity_t* other);

bool pa_playerTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool pa_enemyTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool pa_dummyTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);

void dieWhenFallingOffScreen(paEntity_t* self);

void pa_updateDummy(paEntity_t* self);

void updateEntityDead(paEntity_t* self);

void killEnemy(paEntity_t* target);

void turnAroundAtEdgeOfTileHandler(paEntity_t* self);

void pa_playerOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);
void pa_defaultOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

void killPlayer(paEntity_t* self);

void drawEntityTargetTile(paEntity_t* self);

bool pa_hitBlockTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
void pa_updateBreakBlock(paEntity_t* self);
void pa_updateBlockFragment(paEntity_t* self);

#endif
