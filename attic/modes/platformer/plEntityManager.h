#ifndef _PL_ENTITYMANAGER_H_
#define _PL_ENTITYMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "platformer_typedef.h"
#include "plEntity.h"
#include "plTilemap.h"
#include "plGameData.h"
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

struct plEntityManager_t
{
    wsg_t sprites[SPRITESET_SIZE];
    plEntity_t* entities;
    uint8_t activeEntities;

    plEntity_t* viewEntity;
    plEntity_t* playerEntity;

    plTilemap_t* tilemap;
};

//==============================================================================
// Prototypes
//==============================================================================
void pl_initializeEntityManager(plEntityManager_t* entityManager, plTilemap_t* tilemap, plGameData_t* gameData,
                                plSoundManager_t* soundManager);
void pl_loadSprites(plEntityManager_t* entityManager);
void pl_updateEntities(plEntityManager_t* entityManager);
void pl_deactivateAllEntities(plEntityManager_t* entityManager, bool excludePlayer);
void pl_drawEntities(plEntityManager_t* entityManager);
plEntity_t* pl_findInactiveEntity(plEntityManager_t* entityManager);

void pl_viewFollowEntity(plTilemap_t* tilemap, plEntity_t* entity);
plEntity_t* pl_createEntity(plEntityManager_t* entityManager, uint8_t objectIndex, uint16_t x, uint16_t y);
plEntity_t* pl_createPlayer(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createTestObject(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createScrollLockLeft(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createScrollLockRight(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createScrollLockUp(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createScrollLockDown(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createScrollUnlock(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createHitBlock(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createPowerUp(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createWarp(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createDustBunny(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createWasp(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createEnemyBushL2(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createEnemyBushL3(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createDustBunnyL2(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createDustBunnyL3(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createWaspL2(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createWaspL3(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgColBlue(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgColYellow(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgColOrange(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgColPurple(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgColDarkPurple(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgColBlack(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgColNeutralGreen(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgColNeutralDarkRed(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgColNeutralDarkGreen(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* create1up(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createWaveBall(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createCheckpoint(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgmChange5(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgmChange1(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgmChange2(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgmChange3(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgmChange4(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
plEntity_t* createBgmStop(plEntityManager_t* entityManager, uint16_t x, uint16_t y);
void pl_freeEntityManager(plEntityManager_t* entityManager);

#endif
