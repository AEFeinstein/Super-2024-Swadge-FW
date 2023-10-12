#ifndef _PL_ENTITY_H_
#define _PL_ENTITY_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "platformer_typedef.h"
#include "plTilemap.h"
#include "plGameData.h"
#include "plSoundManager.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum {
    ENTITY_PLAYER,
    plEntity_tEST,
    ENTITY_SCROLL_LOCK_LEFT,
    ENTITY_SCROLL_LOCK_RIGHT,
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
} plEntityIndex_t;

//==============================================================================
// Structs
//==============================================================================

typedef void(*pl_updateFunction_t)(struct plEntity_t *self);
typedef void(*pl_collisionHandler_t)(struct plEntity_t *self, struct plEntity_t *other);
typedef bool(*pl_tileCollisionHandler_t)(struct plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
typedef void(*pl_fallOffTileHandler_t)(struct plEntity_t *self);
typedef void(*pl_overlapTileHandler_t)(struct plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty);

struct plEntity_t
{
    bool active;
    //bool important;

    uint8_t type;
    pl_updateFunction_t updateFunction;

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

    uint8_t spriteIndex;
    bool spriteFlipHorizontal;
    bool spriteFlipVertical;
    uint8_t animationTimer;

    plTilemap_t * tilemap;
    plGameData_t * gameData;
    plSoundManager_t * soundManager;

    uint8_t homeTileX;
    uint8_t homeTileY;

    int16_t jumpPower;

    bool visible;
    uint8_t hp;
    int8_t invincibilityFrames;
    uint16_t scoreValue;
    
    //plEntity_t *entities;
    plEntityManager_t *entityManager;

    pl_collisionHandler_t collisionHandler;
    pl_tileCollisionHandler_t tileCollisionHandler;
    pl_fallOffTileHandler_t fallOffTileHandler;
    pl_overlapTileHandler_t overlapTileHandler;
};

//==============================================================================
// Prototypes
//==============================================================================
void pl_initializeEntity(plEntity_t * self, plEntityManager_t * entityManager, plTilemap_t * tilemap, plGameData_t * gameData, plSoundManager_t * soundManager);

void pl_updatePlayer(plEntity_t * self);
void updateTestObject(plEntity_t * self);
void updateHitBlock(plEntity_t * self);

void pl_moveEntityWithTileCollisions(plEntity_t * self);
void defaultFallOffTileHandler(plEntity_t *self);

void despawnWhenOffscreen(plEntity_t *self);

void pl_destroyEntity(plEntity_t *self, bool respawn);

void applyDamping(plEntity_t *self);

void applyGravity(plEntity_t *self);

void animatePlayer(plEntity_t * self);

void pl_detectEntityCollisions(plEntity_t *self);

void pl_playerCollisionHandler(plEntity_t *self, plEntity_t* other);
void pl_enemyCollisionHandler(plEntity_t *self, plEntity_t *other);
void pl_dummyCollisionHandler(plEntity_t *self, plEntity_t *other);

bool pl_playerTileCollisionHandler(plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool pl_enemyTileCollisionHandler(plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool pl_dummyTileCollisionHandler(plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);

void dieWhenFallingOffScreen(plEntity_t *self);

void pl_updateDummy(plEntity_t* self);

void updateScrollLockLeft(plEntity_t* self);
void updateScrollLockRight(plEntity_t* self);
void updateScrollLockUp(plEntity_t* self);
void updateScrollLockDown(plEntity_t* self);
void updateScrollUnlock(plEntity_t* self);

void updateEntityDead(plEntity_t* self);

void updatePowerUp(plEntity_t* self);
void update1up(plEntity_t* self);
void updateWarp(plEntity_t* self);

void updateDustBunny(plEntity_t* self);
void updateDustBunnyL2(plEntity_t* self);
void updateDustBunnyL3(plEntity_t* self);
bool dustBunnyTileCollisionHandler(plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool dustBunnyL2TileCollisionHandler(plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool dustBunnyL3TileCollisionHandler(plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);


void updateWasp(plEntity_t* self);
void updateWaspL2(plEntity_t* self);
void updateWaspL3(plEntity_t* self);
bool waspTileCollisionHandler(plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);

void killEnemy(plEntity_t* target);

void updateBgCol(plEntity_t* self);

void turnAroundAtEdgeOfTileHandler(plEntity_t *self);

void updateEnemyBushL3(plEntity_t* self);

void updateCheckpoint(plEntity_t* self);

void pl_playerOverlapTileHandler(plEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);
void pl_defaultOverlapTileHandler(plEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

void updateBgmChange(plEntity_t* self);

void updateWaveBall(plEntity_t* self);

// bool waveBallTileCollisionHandler(plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
void waveBallOverlapTileHandler(plEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty);
void powerUpCollisionHandler(plEntity_t *self, plEntity_t *other);
void killPlayer(plEntity_t *self);

#endif
