#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "danceNetwork.h"

//==============================================================================
// Constants
//==============================================================================
#define NUM_SPRITES        0 // The number of dn_sprite_t

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    list_t* entities;

    dn_entity_t* p1Team;
    dn_entity_t* p2Team;
} dn_entityManager_t;

//==============================================================================
// Prototypes
//==============================================================================
void dn_initializeEntityManager(dn_entityManager_t* entityManager, dn_gameData_t* gameData);
void dn_updateEntities(dn_entityManager_t* entityManager);
void dn_drawEntity(dn_entity_t* entity, dn_entityManager_t* entityManager, rectangle_t* camera);
void dn_drawEntities(dn_entityManager_t* entityManager, rectangle_t* camera);
void dn_freeData(dn_entity_t* entity);
void dn_destroyAllEntities(dn_entityManager_t* entityManager);
dn_entity_t* dn_createEntity(dn_entityManager_t* entityManager, dn_animationType_t type, bool paused,
                             dn_spriteDef_t spriteIndex, uint8_t gameFramesPerAnimationFrame, uint32_t x, uint32_t y);
void dn_freeEntityManager(dn_entityManager_t* entityManager);