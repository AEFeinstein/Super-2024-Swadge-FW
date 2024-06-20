#ifndef _PA_ENTITYMANAGER_H_
#define _PA_ENTITYMANAGER_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "pango_typedef.h"
#include "paEntity.h"
#include "paTilemap.h"
#include "paGameData.h"
#include "hdw-tft.h"
#include "paSprite.h"
// #include "soundManager.h"

//==============================================================================
// Constants
//==============================================================================
#define MAX_ENTITIES   32
#define SPRITESET_SIZE 25

//==============================================================================
// Structs
//==============================================================================

struct paEntityManager_t
{
    paSprite_t sprites[SPRITESET_SIZE];
    paEntity_t* entities;
    uint8_t activeEntities;

    int16_t activeEnemies;
    int16_t maxEnemies;
    int16_t remainingEnemies;

    paEntity_t* viewEntity;
    paEntity_t* playerEntity;

    paTilemap_t* tilemap;
};

//==============================================================================
// Prototypes
//==============================================================================
void pa_initializeEntityManager(paEntityManager_t* entityManager, paTilemap_t* tilemap, paGameData_t* gameData,
                                paSoundManager_t* soundManager);
void pa_loadSprites(paEntityManager_t* entityManager);
void pa_updateEntities(paEntityManager_t* entityManager);
void pa_deactivateAllEntities(paEntityManager_t* entityManager, bool excludePlayer);
void pa_drawEntities(paEntityManager_t* entityManager);
paEntity_t* pa_findInactiveEntity(paEntityManager_t* entityManager);

void pa_viewFollowEntity(paTilemap_t* tilemap, paEntity_t* entity);
paEntity_t* pa_createEntity(paEntityManager_t* entityManager, uint8_t objectIndex, uint16_t x, uint16_t y);
paEntity_t* pa_createPlayer(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createTestObject(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createScrollLockLeft(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createScrollLockRight(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createScrollLockUp(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createScrollLockDown(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createScrollUnlock(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createHitBlock(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createPowerUp(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createWarp(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createDustBunny(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createWasp(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createEnemyBushL2(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createEnemyBushL3(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createDustBunnyL2(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createDustBunnyL3(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createWaspL2(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createWaspL3(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgColBlue(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgColYellow(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgColOrange(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgColPurple(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgColDarkPurple(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgColBlack(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgColNeutralGreen(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgColNeutralDarkRed(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgColNeutralDarkGreen(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* create1up(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createWaveBall(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createCheckpoint(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgmChange5(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgmChange1(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgmChange2(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgmChange3(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgmChange4(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* createBgmStop(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
void pa_freeEntityManager(paEntityManager_t* entityManager);
paEntity_t* pa_spawnEnemyFromSpawnBlock(paEntityManager_t* entityManager);
paEntity_t* pa_createBreakBlock(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* pa_createBlockFragment(paEntityManager_t* entityManager, uint16_t x, uint16_t y);

#endif
