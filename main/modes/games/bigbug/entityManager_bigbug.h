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
#include "swadge2024.h"

//==============================================================================
// Constants
//==============================================================================
#define MAX_ENTITIES       200
#define MAX_FRONT_ENTITIES 10
#define NUM_SPRITES        34 // The number of bb_sprite_t last accounted for BB_HOTDOG

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    bb_sprite_t sprites[NUM_SPRITES];
    bb_entity_t* entities;
    bb_entity_t* frontEntities; // important entities that render on top. i.e. dialogue, pango & friends, booster
                                // rocket, garbotnik
    list_t* cachedEntities;
    uint8_t activeEntities;

    bb_entity_t* viewEntity;
    bb_entity_t* playerEntity;
    bb_entity_t* deathDumpster;
    bb_entity_t* boosterEntities[3]; // boosters for three lives
    bb_entity_t* activeBooster;      // the currently active booster
} bb_entityManager_t;

//==============================================================================
// Prototypes
//==============================================================================
void bb_initializeEntityManager(bb_entityManager_t* entityManager, bb_gameData_t* gameData);
bb_sprite_t* bb_loadSprite(const char name[], uint8_t num_frames, uint8_t brightnessLevels, bb_sprite_t* sprite);
void bb_freeSprite(bb_sprite_t* sprite);
void bb_loadSprites(bb_entityManager_t* entityManager);
void bb_updateEntities(bb_entityManager_t* entityManager, bb_camera_t* camera);
void bb_updateStarField(bb_entityManager_t* entityManager, bb_camera_t* camera);
void bb_deactivateNonPersistentEntities(bb_entityManager_t* entityManager);
void bb_deactivateAllEntities(bb_entityManager_t* entityManager, bool excludePlayer);
void bb_drawEntity(bb_entity_t* currentEntity, bb_entityManager_t* entityManager, rectangle_t* camera);
void bb_drawEntities(bb_entityManager_t* entityManager, rectangle_t* camera);
bb_entity_t* bb_findInactiveFrontEntity(bb_entityManager_t* entityManager);
bb_entity_t* bb_findInactiveEntity(bb_entityManager_t* entityManager);
bb_entity_t* bb_findInactiveEntityBackwards(bb_entityManager_t* entityManager);
void bb_ensureEntitySpace(bb_entityManager_t* entityManager, uint8_t numEntities);

void bb_viewFollowEntity(bb_entity_t* entity, bb_camera_t* camera);
bb_entity_t* bb_createEntity(bb_entityManager_t* entityManager, bb_animationType_t type, bool paused,
                             bb_spriteDef_t spriteIndex, uint8_t gameFramesPerAnimationFrame, uint32_t x, uint32_t y,
                             bool renderFront, bool forceToFront);

void bb_freeEntityManager(bb_entityManager_t* entityManager);

#endif