#ifndef _ENTITYMANAGER_BIGBUG_H_
#define _ENTITYMANAGER_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "typedef_bigbug.h"
#include "entity_bigbug.h"
#include "gameData_bigbug.h"
#include "hdw-tft.h"
#include "sprite_bigbug.h"
#include "soundManager_bigbug.h"

//==============================================================================
// Constants
//==============================================================================
#define MAX_ENTITIES   32
#define SPRITESET_SIZE 33

//==============================================================================
// Structs
//==============================================================================

struct bb_entityManager_t
{
    bb_sprite_t sprites[SPRITESET_SIZE];
    bb_entity_t* entities;
    uint8_t activeEntities;

    bb_entity_t* viewEntity;
    bb_entity_t* playerEntity;
};

//==============================================================================
// Prototypes
//==============================================================================
void bb_initializeEntityManager(bb_entityManager_t* entityManager, bb_gameData_t* gameData,
                             bb_soundManager_t* soundManager);
void bb_loadSprites(bb_entityManager_t* entityManager);
void bb_updateEntities(bb_entityManager_t* entityManager);
void bb_deactivateAllEntities(bb_entityManager_t* entityManager, bool excludePlayer, bool excludePersistent, bool respawn);
void bb_drawEntities(bb_entityManager_t* entityManager);
bb_entity_t* bb_findInactiveEntity(bb_entityManager_t* entityManager);

void bb_viewFollowEntity(bb_entity_t* entity);
bb_entity_t* bb_createEntity(bb_entityManager_t* entityManager, uint8_t objectIndex, uint16_t x, uint16_t y);

void bb_freeEntityManager(bb_entityManager_t* entityManager);

#endif