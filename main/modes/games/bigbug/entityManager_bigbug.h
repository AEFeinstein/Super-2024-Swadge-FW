#ifndef _ENTITYMANAGER_BIGBUG_H_
#define _ENTITYMANAGER_BIGBUG_H_

//==============================================================================
// Includes
//==============================================================================

#include <stdint.h>
#include <stdbool.h>
#include "typedef_bigbug.h"
#include "hdw-tft.h"
#include "linked_list.h"
#include "sprite_bigbug.h"
#include "soundManager_bigbug.h"
#include "swadge2024.h"

//==============================================================================
// Constants
//==============================================================================
#define MAX_ENTITIES 32
#define NUM_SPRITES  5 // The number of bb_sprite_t

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    bb_sprite_t sprites[NUM_SPRITES];
    // list_t* sprites[NUM_SPRITES];
    bb_entity_t* entities;
    uint8_t activeEntities;

    bb_entity_t* viewEntity;
    bb_entity_t* playerEntity;
}bb_entityManager_t;

//==============================================================================
// Prototypes
//==============================================================================
void bb_initializeEntityManager(bb_entityManager_t* entityManager, bb_gameData_t* gameData,
                                bb_soundManager_t* soundManager);
bb_sprite_t* bb_loadSprite(const char name[], uint8_t num_frames, bb_sprite_t* sprite);
void bb_loadSprites(bb_entityManager_t* entityManager);
void bb_updateEntities(bb_entityManager_t* entityManager, rectangle_t* camera);
void bb_deactivateAllEntities(bb_entityManager_t* entityManager, bool excludePlayer, bool excludePersistent,
                              bool respawn);
void bb_drawEntities(bb_entityManager_t* entityManager, rectangle_t* camera);
bb_entity_t* bb_findInactiveEntity(bb_entityManager_t* entityManager);

void bb_viewFollowEntity(bb_entity_t* entity, rectangle_t* camera);
bb_entity_t* bb_createEntity(bb_entityManager_t* entityManager, bb_animationType_t type, bool paused, bb_spriteDef_t spriteIndex,
                            uint8_t gameFramesPerAnimationFrame, uint32_t x, uint32_t y);

void bb_freeEntityManager(bb_entityManager_t* entityManager);

#endif