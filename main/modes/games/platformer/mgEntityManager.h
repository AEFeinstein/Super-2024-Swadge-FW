#ifndef _MG_ENTITYMANAGER_H_
#define _MG_ENTITYMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "mega_pulse_ex_typedef.h"
#include "mgWsgManager.h"
#include "mgEntity.h"
#include "mgTilemap.h"
#include "mgGameData.h"
#include "hdw-tft.h"
// #include "soundManager.h"

//==============================================================================
// Constants
//==============================================================================
#define MAX_ENTITIES   32
#define SPRITESET_SIZE 51

//==============================================================================
// Structs
//==============================================================================

struct mgEntityManager_t
{
    mgWsgManager_t* wsgManager;
    mgEntity_t* entities;
    uint8_t activeEntities;

    mgEntity_t* viewEntity;
    mgEntity_t* playerEntity;
    mgEntity_t* bossEntity;
    
    uint16_t bossSpawnX; // Need to store to spawn more bosses in the boss rush.
    uint16_t bossSpawnY; // Need to store to spawn more bosses in the boss rush.

    mgTilemap_t* tilemap;
};

//==============================================================================
// Prototypes
//==============================================================================
void mg_initializeEntityManager(mgEntityManager_t* entityManager, mgWsgManager_t* wsgManager, mgTilemap_t* tilemap,
                                mgGameData_t* gameData, mgSoundManager_t* soundManager);
void mg_updateEntities(mgEntityManager_t* entityManager);
void mg_deactivateAllEntities(mgEntityManager_t* entityManager, bool excludePlayer);
void mg_deactivateAllBullets(mgEntityManager_t* entityManager);
void mg_drawEntities(mgEntityManager_t* entityManager);
mgEntity_t* mg_findInactiveEntity(mgEntityManager_t* entityManager);

void mg_viewFollowEntity(mgTilemap_t* tilemap, mgEntity_t* entity);
mgEntity_t* mg_createEntity(mgEntityManager_t* entityManager, uint8_t objectIndex, uint16_t x, uint16_t y);
mgEntity_t* mg_createPlayer(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createTestObject(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createScrollLockLeft(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createScrollLockRight(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createScrollLockUp(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createScrollLockDown(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createScrollUnlock(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createHitBlock(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createPowerUp(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createWarp(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createDustBunny(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createWasp(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createEnemyBushL2(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createEnemyBushL3(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createDustBunnyL2(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createDustBunnyL3(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createWaspL2(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createWaspL3(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgColBlue(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgColYellow(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgColOrange(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgColPurple(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgColDarkPurple(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgColBlack(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgColNeutralGreen(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgColNeutralDarkRed(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgColNeutralDarkGreen(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* create1up(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createWaveBall(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createCheckpoint(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgmChange5(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgmChange1(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgmChange2(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgmChange3(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgmChange4(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBgmStop(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createWarpEntranceWall(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createWarpEntranceFloor(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createCharginSchmuck(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBouncinSchmuck(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createTurret(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createSheldonShieldy(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
// mgEntity_t* createSpawnerBot(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createSpikyMcGee(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createAirTurret(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBouncepad(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBouncepadDiagonal(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createLifeRefillSmall(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createLifeRefillLarge(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossSeverYagata(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossSmashGorilla(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossGrindPangolin(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossDrainBat(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossKineticDonut(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossTrashMan(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossFlareGryffyn(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossDeadeyeChirpzi(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossBigma(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossHankWaddle(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createMixtape(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createBossDoor(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
mgEntity_t* createShrubbleLv4(mgEntityManager_t* entityManager, uint16_t x, uint16_t y);
void mg_freeEntityManager(mgEntityManager_t* entityManager);

#endif
