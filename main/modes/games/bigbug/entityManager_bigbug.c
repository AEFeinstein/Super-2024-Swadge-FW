//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>

#include "entityManager_bigbug.h"
#include "esp_random.h"
#include "palette.h"

#include "hdw-spiffs.h"
#include "spiffs_wsg.h"

//==============================================================================
// Constants
//==============================================================================
#define SUBPIXEL_RESOLUTION 4

//==============================================================================
// Functions
//==============================================================================
void bb_initializeEntityManager(bb_entityManager_t* entityManager, bb_gameData_t* gameData,
                             bb_soundManager_t* soundManager)
{
    bb_loadSprites(entityManager);
    entityManager->entities = calloc(MAX_ENTITIES, sizeof(bb_entity_t));

    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_initializeEntity(&(entityManager->entities[i]), entityManager, gameData, soundManager);
    }

    entityManager->activeEntities = 0;

    // entityManager->viewEntity = createPlayer(entityManager, entityManager->tilemap->warps[0].x * 16,
    // entityManager->tilemap->warps[0].y * 16); entityManager->playerEntity = entityManager->viewEntity;
}

void bb_loadSprites(bb_entityManager_t* entityManager)
{
}

void bb_updateEntities(bb_entityManager_t* entityManager)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        if (entityManager->entities[i].active)
        {
            entityManager->entities[i].updateFunction(&(entityManager->entities[i]));

            if (&(entityManager->entities[i]) == entityManager->viewEntity)
            {
                bb_viewFollowEntity(&(entityManager->entities[i]));
            }
        }
    }
}

void bb_deactivateAllEntities(bb_entityManager_t* entityManager, bool excludePlayer, bool excludePersistent, bool respawn)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_entity_t* currentEntity = &(entityManager->entities[i]);
        if (!currentEntity->active || (excludePersistent && currentEntity->persistent))
        {
            continue;
        }

        bb_destroyEntity(currentEntity, respawn);

        if (excludePlayer && currentEntity == entityManager->playerEntity)
        {
            currentEntity->active = true;
        }
    }
}

void bb_drawEntities(bb_entityManager_t* entityManager)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_entity_t currentEntity = entityManager->entities[i];

        if (currentEntity.active && currentEntity.visible)
        {
            drawWsg(&(entityManager->sprites[currentEntity.spriteIndex].wsg),
                    (currentEntity.x >> SUBPIXEL_RESOLUTION) - entityManager->sprites[currentEntity.spriteIndex].originX,
                    (currentEntity.y >> SUBPIXEL_RESOLUTION)
                        - entityManager->sprites[currentEntity.spriteIndex].originY,
                    currentEntity.spriteFlipHorizontal, currentEntity.spriteFlipVertical,
                    currentEntity.spriteRotateAngle);
        }
    }
}

bb_entity_t* bb_findInactiveEntity(bb_entityManager_t* entityManager)
{
    if (entityManager->activeEntities == MAX_ENTITIES)
    {
        return NULL;
    };

    uint8_t entityIndex = 0;

    while (entityManager->entities[entityIndex].active)
    {
        entityIndex++;

        // Extra safeguard to make sure we don't get stuck here
        if (entityIndex >= MAX_ENTITIES)
        {
            return NULL;
        }
    }

    return &(entityManager->entities[entityIndex]);
}

void bb_viewFollowEntity(bb_entity_t* entity)
{
    int16_t moveViewByX = (entity->x) >> SUBPIXEL_RESOLUTION;
    int16_t moveViewByY = (entity->y > 63616) ? 0 : (entity->y) >> SUBPIXEL_RESOLUTION;
}

bb_entity_t* bb_createEntity(bb_entityManager_t* entityManager, uint8_t objectIndex, uint16_t x, uint16_t y)
{
    if (entityManager->activeEntities == MAX_ENTITIES)
    {
        return NULL;
    }

    bb_entity_t* createdEntity;

    switch (objectIndex)
    {
        default:
            createdEntity = NULL;
    }

    if (createdEntity != NULL)
    {
        entityManager->activeEntities++;
    }

    return createdEntity;
}

void bb_freeEntityManager(bb_entityManager_t* self)
{
    free(self->entities);
    for (uint8_t i = 0; i < SPRITESET_SIZE; i++)
    {
        freeWsg(&(self->sprites[i].wsg));
    }
}