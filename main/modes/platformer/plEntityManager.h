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
#include "display.h"

//==============================================================================
// Constants
//==============================================================================
#define MAX_ENTITIES 32
#define SPRITESET_SIZE 51

//==============================================================================
// Structs
//==============================================================================

struct entityManager_t
{
    wsg_t sprites[SPRITESET_SIZE];
    entity_t * entities;
    uint8_t activeEntities;

    entity_t * viewEntity;
    entity_t * playerEntity;

    tilemap_t * tilemap;
};

//==============================================================================
// Prototypes
//==============================================================================
void initializeEntityManager(entityManager_t * entityManager, tilemap_t * tilemap, gameData_t * gameData);
void loadSprites(entityManager_t * entityManager);
void updateEntities(entityManager_t * entityManager);
void deactivateAllEntities(entityManager_t * entityManager, bool excludePlayer);
void drawEntities(display_t * disp, entityManager_t * entityManager);
entity_t * findInactiveEntity(entityManager_t * entityManager);

void viewFollowEntity(tilemap_t * tilemap, entity_t * entity);
entity_t* createEntity(entityManager_t *entityManager, uint8_t objectIndex, uint16_t x, uint16_t y);
entity_t* createPlayer(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createTestObject(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createScrollLockLeft(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createScrollLockRight(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createScrollLockUp(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createScrollLockDown(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createScrollUnlock(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createHitBlock(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createPowerUp(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createWarp(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createDustBunny(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createWasp(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createEnemyBushL2(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createEnemyBushL3(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createDustBunnyL2(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createDustBunnyL3(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createWaspL2(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createWaspL3(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgColBlue(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgColYellow(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgColOrange(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgColPurple(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgColDarkPurple(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgColBlack(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgColNeutralGreen(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgColNeutralDarkRed(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgColNeutralDarkGreen(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* create1up(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createWaveBall(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createCheckpoint(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgmChange5(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgmChange1(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgmChange2(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgmChange3(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgmChange4(entityManager_t * entityManager, uint16_t x, uint16_t y);
entity_t* createBgmStop(entityManager_t * entityManager, uint16_t x, uint16_t y);
void freeEntityManager(entityManager_t * entityManager);












#endif
