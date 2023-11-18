#ifndef _ENTITY_H_
#define _ENTITY_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "breakout_typedef.h"
#include "tilemap.h"
#include "gameData.h"
#include "soundManager.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    ENTITY_PLAYER_PADDLE_BOTTOM,
    ENTITY_PLAYER_PADDLE_TOP,
    ENTITY_PLAYER_PADDLE_LEFT,
    ENTITY_PLAYER_PADDLE_RIGHT,
    ENTITY_PLAYER_TIME_BOMB,
    ENTITY_PLAYER_REMOTE_BOMB,
    ENTITY_PLAYER_BOMB_EXPLOSION,
    ENTITY_BALL_TRAIL,
    ENTITY_CHO_INTRO,
    ENTITY_CRAWLER,
    ENTITY_UNUSED_10,
    ENTITY_UNUSED_11,
    ENTITY_UNUSED_12,
    ENTITY_UNUSED_13,
    ENTITY_UNUSED_14,
    ENTITY_UNUSED_15,
    ENTITY_PLAYER_BALL,
    ENTITY_CAPTIVE_BALL,
    ENTITY_UNUSED_18,
    ENTITY_UNUSED_19,
    ENTITY_UNUSED_20,
    ENTITY_UNUSED_21
} entityIndex_t;

typedef enum
{
    CRAWLER_TOP_TO_RIGHT,
    CRAWLER_RIGHT_TO_BOTTOM,
    CRAWLER_BOTTOM_TO_LEFT,
    CRAWLER_LEFT_TO_TOP,
    CRAWLER_TOP_TO_LEFT,
    CRAWLER_RIGHT_TO_TOP,
    CRAWLER_BOTTOM_TO_RIGHT,
    CRAWLER_LEFT_TO_BOTTOM
} crawlerMoveState_t;

//==============================================================================
// Structs
//==============================================================================

typedef void (*updateFunction_t)(struct entity_t* self);
typedef void (*collisionHandler_t)(struct entity_t* self, struct entity_t* other);
typedef bool (*tileCollisionHandler_t)(struct entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty,
                                       uint8_t direction);
typedef void (*fallOffTileHandler_t)(struct entity_t* self);
typedef void (*overlapTileHandler_t)(struct entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

struct entity_t
{
    bool active;
    bool persistent;

    uint8_t type;
    updateFunction_t updateFunction;

    uint16_t x;
    uint16_t y;

    int16_t xspeed;
    int16_t yspeed;

    uint8_t spriteIndex;
    bool spriteFlipHorizontal;
    bool spriteFlipVertical;
    int16_t spriteRotateAngle;

    uint8_t animationTimer;

    tilemap_t* tilemap;
    gameData_t* gameData;
    soundManager_t* soundManager;

    uint8_t homeTileX;
    uint8_t homeTileY;

    bool visible;

    entity_t* attachedToEntity;

    bool shouldAdvanceMultiplier;
    int16_t bouncesOffUnbreakableBlocks;
    int16_t breakInfiniteLoopBounceThreshold;

    int16_t baseSpeed;
    int16_t maxSpeed;
    int16_t bouncesToNextSpeedUp;
    uint8_t speedUpLookupIndex;

    entityManager_t* entityManager;

    collisionHandler_t collisionHandler;
    tileCollisionHandler_t tileCollisionHandler;
    overlapTileHandler_t overlapTileHandler;
};

//==============================================================================
// Prototypes
//==============================================================================
void initializeEntity(entity_t* self, entityManager_t* entityManager, tilemap_t* tilemap, gameData_t* gameData,
                      soundManager_t* soundManager);

void updatePlayer(entity_t* self);
void updatePlayerVertical(entity_t* self);

void updateBall(entity_t* self);
void updateBallAtStart(entity_t* self);
bool isOutsidePlayfield(entity_t* self);
void detectLostBall(entity_t* self, bool respawn);
void updateCaptiveBallNotInPlay(entity_t* self);
void updateCaptiveBallInPlay(entity_t* self);
uint32_t getTaxiCabDistanceBetweenEntities(entity_t* self, entity_t* other);
void updateTimeBomb(entity_t* self);
void updateRemoteBomb(entity_t* self);
void explodeBomb(entity_t* self);
void updateExplosion(entity_t* self);
void updateBallTrail(entity_t* self);
void updateChoIntro(entity_t* self);
void updateCrawler(entity_t* self);
void crawlerSetMoveState(entity_t* self, uint8_t state);
void crawlerInitMoveState(entity_t* self);

entity_t* findFirstEntityOfType(entityManager_t* entityManager, uint8_t type);
void updateChoLevelClear(entity_t* self);

void moveEntityWithTileCollisions(entity_t* self);

void destroyEntity(entity_t* self, bool respawn);

void detectEntityCollisions(entity_t* self);

void playerCollisionHandler(entity_t* self, entity_t* other);
void crawlerCollisionHandler(entity_t* self, entity_t* other);
void dummyCollisionHandler(entity_t* self, entity_t* other);
void ballCollisionHandler(entity_t* self, entity_t* other);
void captiveBallCollisionHandler(entity_t* self, entity_t* other);
void advanceBallSpeed(entity_t* self, uint16_t factor);

bool playerTileCollisionHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool enemyTileCollisionHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool dummyTileCollisionHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool ballTileCollisionHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool captiveBallTileCollisionHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);

void defaultOverlapTileHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);
void playerOverlapTileHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);
void ballOverlapTileHandler(entity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

int16_t breakBlockTile(tilemap_t* tilemap, gameData_t* gameData, uint8_t tileId, uint8_t tx, uint8_t ty);
void setLedBreakBlock(gameData_t* gameData, uint8_t tileId);

void updateDummy(entity_t* self);
void setVelocity(entity_t* self, int16_t direction, int16_t magnitude);

#endif
