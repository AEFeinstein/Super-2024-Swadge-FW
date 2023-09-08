#ifndef _ENTITY_H_
#define _ENTITY_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "common_typedef.h"
#include "tilemap.h"
#include "gameData.h"
#include "soundManager.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum {
    ENTITY_PLAYER_PADDLE_BOTTOM,
    ENTITY_PLAYER_PADDLE_TOP,
    ENTITY_PLAYER_PADDLE_LEFT,
    ENTITY_PLAYER_PADDLE_RIGHT,
    ENTITY_UNUSED_4,
    ENTITY_UNUSED_5,
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
    ENTITY_PLAYER_BALL,
    ENTITY_PLAYER_BOMB,
    ENTITY_PLAYER_BOMB_EXPLOSION
} entityIndex_t;

//==============================================================================
// Structs
//==============================================================================

typedef void(*updateFunction_t)(struct entity_t *self);
typedef void(*collisionHandler_t)(struct entity_t *self, struct entity_t *other);
typedef bool(*tileCollisionHandler_t)(struct entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
typedef void(*fallOffTileHandler_t)(struct entity_t *self);
typedef void(*overlapTileHandler_t)(struct entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty);

struct entity_t
{
    bool active;
    //bool important;

    uint8_t type;
    updateFunction_t updateFunction;

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
    int16_t spriteRotateAngle;

    uint8_t animationTimer;

    tilemap_t * tilemap;
    gameData_t * gameData;
    soundManager_t * soundManager;

    uint8_t homeTileX;
    uint8_t homeTileY;

    int16_t jumpPower;

    bool visible;
    uint8_t hp;
    int8_t invincibilityFrames;
    uint16_t scoreValue;

    entity_t *attachedToEntity;
    bool shouldAdvanceMultiplier;
    
    //entity_t *entities;
    entityManager_t *entityManager;

    collisionHandler_t collisionHandler;
    tileCollisionHandler_t tileCollisionHandler;
    fallOffTileHandler_t fallOffTileHandler;
    overlapTileHandler_t overlapTileHandler;
};

//==============================================================================
// Prototypes
//==============================================================================
void initializeEntity(entity_t * self, entityManager_t * entityManager, tilemap_t * tilemap, gameData_t * gameData, soundManager_t * soundManager);

void updatePlayer(entity_t * self);
void updatePlayerVertical(entity_t * self);

void updateBall(entity_t * self);
void updateBallAtStart(entity_t *self);
void updateBomb(entity_t * self);
void updateExplosion(entity_t * self);

void updateHitBlock(entity_t * self);


void moveEntityWithTileCollisions(entity_t * self);
void defaultFallOffTileHandler(entity_t *self);

void despawnWhenOffscreen(entity_t *self);

void destroyEntity(entity_t *self, bool respawn);

void applyDamping(entity_t *self);

void applyGravity(entity_t *self);

void animatePlayer(entity_t * self);

void detectEntityCollisions(entity_t *self);

void playerCollisionHandler(entity_t *self, entity_t* other);
void enemyCollisionHandler(entity_t *self, entity_t *other);
void dummyCollisionHandler(entity_t *self, entity_t *other);
void ballCollisionHandler(entity_t *self, entity_t *other);

bool playerTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool enemyTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool dummyTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool ballTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
void ballOverlapTileHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

void breakBlockTile(tilemap_t *tilemap, gameData_t *gameData, uint8_t tileId, uint8_t tx, uint8_t ty);
void setLedBreakBlock(gameData_t *gameData, uint8_t tileId);

void dieWhenFallingOffScreen(entity_t *self);

void updateDummy(entity_t* self);
void setVelocity(entity_t *self, int16_t direction, int16_t magnitude);

void updateScrollLockLeft(entity_t* self);
void updateScrollLockRight(entity_t* self);
void updateScrollLockUp(entity_t* self);
void updateScrollLockDown(entity_t* self);
void updateScrollUnlock(entity_t* self);

void updateEntityDead(entity_t* self);

void updatePowerUp(entity_t* self);
void update1up(entity_t* self);
void updateWarp(entity_t* self);

void updateDustBunny(entity_t* self);
void updateDustBunnyL2(entity_t* self);
void updateDustBunnyL3(entity_t* self);
bool dustBunnyTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool dustBunnyL2TileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool dustBunnyL3TileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);


void updateWasp(entity_t* self);
void updateWaspL2(entity_t* self);
void updateWaspL3(entity_t* self);
bool waspTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);

void killEnemy(entity_t* target);

void updateBgCol(entity_t* self);

void turnAroundAtEdgeOfTileHandler(entity_t *self);

void updateEnemyBushL3(entity_t* self);

void updateCheckpoint(entity_t* self);

void playerOverlapTileHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);
void defaultOverlapTileHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

void updateBgmChange(entity_t* self);

void updateWaveBall(entity_t* self);

// bool waveBallTileCollisionHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
void waveBallOverlapTileHandler(entity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty);
void powerUpCollisionHandler(entity_t *self, entity_t *other);
void killPlayer(entity_t *self);

#endif
