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
    PA_ENTITY_TEST,
    PA_ENTITY_BREAK_BLOCK,
    PA_ENTITY_BLOCK_FRAGMENT,
    ENTITY_SCROLL_LOCK_UP,
    ENTITY_SCROLL_LOCK_DOWN,
    ENTITY_SCROLL_UNLOCK,
    ENTITY_HIT_BLOCK,
    ENTITY_DEAD,
    ENTITY_POWERUP,
    ENTITY_WARP,
    ENTITY_DUST_BUNNY,
    ENTITY_WASP,
    ENTITY_BUSH_2,
    ENTITY_BUSH_3,
    ENTITY_DUST_BUNNY_2,
    ENTITY_DUST_BUNNY_3,
    ENTITY_WASP_2,
    ENTITY_WASP_3,
    ENTITY_BGCOL_BLUE,
    ENTITY_BGCOL_YELLOW,
    ENTITY_BGCOL_ORANGE,
    ENTITY_BGCOL_PURPLE,
    ENTITY_BGCOL_DARK_PURPLE,
    ENTITY_BGCOL_BLACK,
    ENTITY_BGCOL_NEUTRAL_GREEN,
    ENTITY_BGCOL_DARK_RED,
    ENTITY_BGCOL_DARK_GREEN,
    ENTITY_1UP,
    ENTITY_WAVE_BALL,
    ENTITY_CHECKPOINT,
    ENTITY_BGM_STOP,
    ENTITY_BGM_CHANGE_1,
    ENTITY_BGM_CHANGE_2,
    ENTITY_BGM_CHANGE_3,
    ENTITY_BGM_CHANGE_4,
    ENTITY_BGM_CHANGE_5
} paEntityIndex_t;

typedef enum
{
    PA_DIRECTION_LEFT,
    PA_DIRECTION_RIGHT,
    PA_DIRECTION_UP,
    PA_DIRECTION_NONE,
    PA_DIRECTION_DOWN
} paCardinalDirection_t;

typedef enum
{
    PA_EN_ST_SPAWNING,
    PA_EN_ST_STUN,
    PA_EN_ST_NORMAL,
    PA_EN_ST_AGGRESSIVE,
    PA_EN_ST_BREAK_BLOCK,
} paEnemyState_t;

//==============================================================================
// Structs
//==============================================================================

typedef void (*pa_updateFunction_t)(struct paEntity_t* self);
typedef void (*pa_collisionHandler_t)(struct paEntity_t* self, struct paEntity_t* other);
typedef bool (*PA_TILE_CollisionHandler_t)(struct paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty,
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

    int16_t jumpPower;

    bool visible;
    uint8_t hp;
    int8_t invincibilityFrames;
    uint16_t scoreValue;

    uint8_t targetTileX;
    uint8_t targetTileY;
    uint16_t state;

    // paEntity_t *entities;
    paEntityManager_t* entityManager;

    pa_collisionHandler_t collisionHandler;
    PA_TILE_CollisionHandler_t tileCollisionHandler;
    pa_fallOffTileHandler_t fallOffTileHandler;
    pa_overlapTileHandler_t overlapTileHandler;
};

//==============================================================================
// Prototypes
//==============================================================================
void pa_initializeEntity(paEntity_t* self, paEntityManager_t* entityManager, paTilemap_t* tilemap,
                         paGameData_t* gameData, paSoundManager_t* soundManager);

void pa_updatePlayer(paEntity_t* self);
void updateTestObject(paEntity_t* self);
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

void updateScrollLockLeft(paEntity_t* self);
void updateScrollLockRight(paEntity_t* self);
void updateScrollLockUp(paEntity_t* self);
void updateScrollLockDown(paEntity_t* self);
void updateScrollUnlock(paEntity_t* self);

void updateEntityDead(paEntity_t* self);

void updatePowerUp(paEntity_t* self);
void update1up(paEntity_t* self);
void updateWarp(paEntity_t* self);

void updateDustBunny(paEntity_t* self);
void updateDustBunnyL2(paEntity_t* self);
void updateDustBunnyL3(paEntity_t* self);
bool dustBunnyTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool dustBunnyL2TileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool dustBunnyL3TileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);

void updateWasp(paEntity_t* self);
void updateWaspL2(paEntity_t* self);
void updateWaspL3(paEntity_t* self);
bool waspTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);

void killEnemy(paEntity_t* target);

void updateBgCol(paEntity_t* self);

void turnAroundAtEdgeOfTileHandler(paEntity_t* self);

void updateEnemyBushL3(paEntity_t* self);

void updateCheckpoint(paEntity_t* self);

void pa_playerOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);
void pa_defaultOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

void updateBgmChange(paEntity_t* self);

void updateWaveBall(paEntity_t* self);

// bool waveBallTileCollisionHandler(paEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
void waveBallOverlapTileHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);
void powerUpCollisionHandler(paEntity_t* self, paEntity_t* other);
void killPlayer(paEntity_t* self);

void drawEntityTargetTile(paEntity_t* self);

bool pa_hitBlockTileCollisionHandler(paEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
void pa_updateBreakBlock(paEntity_t* self);
void pa_updateBlockFragment(paEntity_t* self);

#endif
