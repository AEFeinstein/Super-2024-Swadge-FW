#ifndef _MG_ENTITY_H_
#define _MG_ENTITY_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "mega_pulse_ex_typedef.h"
#include "mgTilemap.h"
#include "mgGameData.h"
#include "mgSoundManager.h"
#include "mgEntitySpawnData.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    ENTITY_PLAYER,
    mgEntity_tEST,
    ENTITY_SCROLL_LOCK_LEFT,
    ENTITY_SCROLL_LOCK_RIGHT,
    ENTITY_SCROLL_LOCK_UP,
    ENTITY_SCROLL_LOCK_DOWN,
    ENTITY_SCROLL_UNLOCK,
    ENTITY_HIT_BLOCK,
    ENTITY_DEAD,
    ENTITY_POWERUP,
    ENTITY_WARP,
    ENTITY_DUST_BUNNY, // ENTITY_JUMPRABBIT
    ENTITY_WASP,       // ENTITY_SWOOPER
    ENTITY_BUSH_2,
    ENTITY_BUSH_3,
    ENTITY_DUST_BUNNY_2,
    ENTITY_DUST_BUNNY_3,
    ENTITY_WASP_2, // ENTITY_SWOOPER_PLUS
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
    ENTITY_BGM_CHANGE_5,
    ENTITY_WARP_ENTRANCE_WALL,
    ENTITY_WARP_ENTRANCE_FLOOR,
    ENTITY_WARP_EXIT_WALL,
    ENTITY_WARP_EXIT_FLOOR,
    ENTITY_CHARGIN_SCHMUCK,
    ENTITY_BOUNCIN_SCHMUCK,
    ENTITY_TURRET,
    ENTITY_SHELDON_SHIELDY,
    ENTITY_SPAWNER_BOT,
    ENTITY_SPIKY_MCGEE,
    ENTITY_AIR_TURRET,
    ENTITY_BOUNCE_PAD,
    ENTITY_BOUNCE_PAD_DIAGONAL,
    ENTITY_LIFE_REFILL_SMALL,
    ENTITY_LIFE_REFILL_LARGE,
    ENTITY_BOSS_SEVER_YATAGA,
    ENTITY_MIXTAPE,
    ENTITY_BOSS_DOOR,
    ENTITY_SHRUBBLE_LV4,
    ENTITY_BOSS_SMASH_GORILLA,
    ENTITY_BOSS_GRIND_PANGOLIN,
    ENTITY_BOSS_DRAIN_BAT,
    ENTITY_BOSS_KINETIC_DONUT,
    ENTITY_BOSS_TRASH_MAN,
    ENTITY_BOSS_FLARE_GRYFFYN,
    ENTITY_BOSS_DEADEYE_CHIRPZI,
    ENTITY_BOSS_BIGMA,
    ENTITY_BOSS_HANK_WADDLE,
    ENTITY_EXTRA_LIFE,
} mgEntityIndex_t;

typedef enum
{
    MG_PL_ST_NORMAL,
    MG_PL_ST_DASHING,
    MG_PL_ST_MIC_DROP,
    MG_PL_ST_UPPERCUT,
    MG_PL_ST_HURT,
    MG_PL_ST_SHIELD
} mgPlayerState_t;

typedef enum
{
    CRAWLER_NONE,
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

typedef void (*mg_updateFunction_t)(struct mgEntity_t* self);
typedef void (*mg_collisionHandler_t)(struct mgEntity_t* self, struct mgEntity_t* other);
typedef bool (*mg_tileCollisionHandler_t)(struct mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty,
                                          uint8_t direction);
typedef void (*mg_fallOffTileHandler_t)(struct mgEntity_t* self);
typedef void (*mg_overlapTileHandler_t)(struct mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);
typedef void (*mg_drawHandler_t)(struct mgEntity_t* self);

struct mgEntity_t
{
    bool active;
    // bool important;

    uint8_t type;
    mg_updateFunction_t updateFunction;

    uint16_t x;
    uint16_t y;

    int16_t xspeed;
    int16_t yspeed;

    int16_t xMaxSpeed;
    int16_t yMaxSpeed;

    int16_t xDamping;
    int16_t yDamping;

    /* Generic special purpose fields used by some entities for boss waves etc. */
    uint8_t special1;
    int16_t specialX;
    int16_t specialN;

    bool gravityEnabled;
    int16_t gravity;
    bool falling;

    uint8_t spriteIndex;
    bool spriteFlipHorizontal;
    bool spriteFlipVertical;
    int16_t spriteRotateAngle;
    uint8_t animationTimer;
    uint8_t doubleJumpAnimTimer; /* counts down frames for the double-jump animation */

    mgTilemap_t* tilemap;
    mgGameData_t* gameData;
    mgSoundManager_t* soundManager;

    uint8_t homeTileX;
    uint8_t homeTileY;

    uint16_t state;
    int16_t stateTimer;

    int16_t jumpPower;
    bool canDash;
    int8_t shotsFired;
    int8_t shotLimit;

    bool visible;
    int16_t hp;
    int8_t invincibilityFrames;
    uint16_t scoreValue;
    // uint16_t damageValue;

    mgEntitySpawnData_t* spawnData;
    mgEntity_t* linkedEntity;

    // mgEntity_t *entities;
    mgEntityManager_t* entityManager;

    mg_collisionHandler_t collisionHandler;
    mg_tileCollisionHandler_t tileCollisionHandler;
    mg_fallOffTileHandler_t fallOffTileHandler;
    mg_overlapTileHandler_t overlapTileHandler;
    mg_drawHandler_t drawHandler;

    const mg_EntityTileCollider_t* tileCollider;
};

//==============================================================================
// Prototypes
//==============================================================================
void mg_initializeEntity(mgEntity_t* self, mgEntityManager_t* entityManager, mgTilemap_t* tilemap,
                         mgGameData_t* gameData, mgSoundManager_t* soundManager);

void mg_updatePlayer(mgEntity_t* self);
void updateTestObject(mgEntity_t* self);
void mg_updateInvincibilityFrames(mgEntity_t* self);
void updateHitBlock(mgEntity_t* self);

void mg_moveEntityWithTileCollisions(mgEntity_t* self);
void mg_moveEntityWithTileCollisions3(mgEntity_t* self);
bool mg_canWallJump(mgEntity_t* self);
bool mg_canExitDashSlide(mgEntity_t* self);
void defaultFallOffTileHandler(mgEntity_t* self);

void mg_playerFallOffTileHandler(mgEntity_t* self);

void despawnWhenOffscreen(mgEntity_t* self);

void mg_bossRushLogic(mgEntity_t* self);

void mg_destroyEntity(mgEntity_t* self, bool respawn);

void applyDamping(mgEntity_t* self);

void applyGravity(mgEntity_t* self);

void animatePlayer(mgEntity_t* self);

void mg_detectEntityCollisions(mgEntity_t* self);

void mg_playerCollisionHandler(mgEntity_t* self, mgEntity_t* other);
void mg_enemyCollisionHandler(mgEntity_t* self, mgEntity_t* other);
void mg_dummyCollisionHandler(mgEntity_t* self, mgEntity_t* other);

bool mg_playerTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool mg_enemyTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool mg_trashManTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool mg_dummyTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);

void dieWhenFallingOffScreen(mgEntity_t* self);

void mg_updateDummy(mgEntity_t* self);

void updateScrollLockLeft(mgEntity_t* self);
void updateScrollLockRight(mgEntity_t* self);
void updateScrollLockUp(mgEntity_t* self);
void updateScrollLockDown(mgEntity_t* self);
void updateScrollUnlock(mgEntity_t* self);

void updateEntityDead(mgEntity_t* self);

void updatePowerUp(mgEntity_t* self);
void updateExtraLife(mgEntity_t* self);
void update1up(mgEntity_t* self);
void updateWarp(mgEntity_t* self);

void updateDustBunny(mgEntity_t* self);
void updateDustBunnyL2(mgEntity_t* self);
void updateDustBunnyL3(mgEntity_t* self);
bool dustBunnyTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool dustBunnyL2TileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
bool dustBunnyL3TileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);

void updateWasp(mgEntity_t* self);
void updateWaspL2(mgEntity_t* self);
void updateWaspL3(mgEntity_t* self);
bool waspTileCollisionHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);

void killEnemy(mgEntity_t* target);

void updateBgCol(mgEntity_t* self);

void turnAroundAtEdgeOfTileHandler(mgEntity_t* self);

void updateEnemyBushL3(mgEntity_t* self);

void updateCheckpoint(mgEntity_t* self);

void mg_playerOverlapTileHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);
void mg_defaultOverlapTileHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);

void updateBgmChange(mgEntity_t* self);

void updateWaveBall(mgEntity_t* self);

// bool waveBallTileCollisionHandler(mgEntity_t *self, uint8_t tileId, uint8_t tx, uint8_t ty, uint8_t direction);
void waveBallOverlapTileHandler(mgEntity_t* self, uint8_t tileId, uint8_t tx, uint8_t ty);
void powerUpCollisionHandler(mgEntity_t* self, mgEntity_t* other);
void killPlayer(mgEntity_t* self);
void mg_defaultEntityDrawHandler(mgEntity_t* self);
void mg_smashGorillaProjectileDrawHandler(mgEntity_t* self);
void mg_hankDrawHandler(mgEntity_t* self);

void mg_playerDrawHandler(mgEntity_t* self);

void mg_destroyShot(mgEntity_t* self);

void mg_updateTurret(mgEntity_t* self);

int16_t clampAngleTo8way(int16_t angle);

void mg_updateCharginSchmuck(mgEntity_t* self);

void mg_enemySightBulletCollisionHandler(mgEntity_t* self, mgEntity_t* other);

void mg_updateBossDoor(mgEntity_t* self);

void mg_bossDoorCollisionHandler(mgEntity_t* self, mgEntity_t* other);

void mg_updateBossSeverYataga(mgEntity_t* self);

void mg_updateBossSmashGorilla(mgEntity_t* self);

void mg_updateBossGrindPangolin(mgEntity_t* self);

void mg_updateBossDrainBat(mgEntity_t* self);

void mg_updateBossKineticDonut(mgEntity_t* self);

void mg_updateBossTrashMan(mgEntity_t* self);

void mg_updateBossFlareGryffyn(mgEntity_t* self);

void mg_updateBossDeadeyeChirpzi(mgEntity_t* self);

void mg_updateBossBigma(mgEntity_t* self);

void mg_updateBossHankWaddle(mgEntity_t* self);

void mg_updateShrubbleLv4(mgEntity_t* self);

void crawlerSetMoveState(mgEntity_t* self, uint8_t state);

uint8_t mg_crawlerGettInitialMoveState(int16_t angle, bool counterclockwise);

#endif
