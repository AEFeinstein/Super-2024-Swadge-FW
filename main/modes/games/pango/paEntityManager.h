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
#define SPRITESET_SIZE 32

//==============================================================================
// Structs
//==============================================================================

struct paEntityManager_t
{
    paSprite_t sprites[SPRITESET_SIZE];
    paEntity_t* entities;
    uint8_t activeEntities;

    int16_t activeEnemies;

    paGameData_t* gameData;

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
paEntity_t* createHitBlock(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
void pa_freeEntityManager(paEntityManager_t* entityManager);
paEntity_t* pa_spawnEnemyFromSpawnBlock(paEntityManager_t* entityManager);
paEntity_t* pa_createBreakBlock(paEntityManager_t* entityManager, uint16_t x, uint16_t y);
paEntity_t* pa_createBlockFragment(paEntityManager_t* entityManager, uint16_t x, uint16_t y);

#endif
