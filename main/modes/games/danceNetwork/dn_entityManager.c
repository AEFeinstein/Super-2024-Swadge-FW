
//==============================================================================
// Includes
//==============================================================================

#include "dn_entityManager.h"
#include "dn_entity.h"
#include "linked_list.h"

//==============================================================================
// Functions
//==============================================================================
void dn_initializeEntityManager(dn_entityManager_t* entityManager, dn_gameData_t* gameData)
{
    bb_loadSprites(entityManager);
    entityManager->entities = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "entities");
}

void dn_updateEntities(dn_entityManager_t* entityManager)
{
    node_t* curNode = entityManager->entities->first;
    while (curNode != NULL)
    {
        dn_entity_t* entity = (dn_entity_t*)curNode->val;
        if (entity->updateFunction != NULL)
        {
            entity->updateFunction(entity);
        }
        curNode = curNode->next;
    }
}

void dn_drawEntity(dn_entity_t* entity, dn_entityManager_t* entityManager, rectangle_t* camera)
{
    if(entity->drawFunction == NULL) return;
    entity->drawFunction(entityManager, camera, entity);
}

void dn_drawEntities(dn_entityManager_t* entityManager, rectangle_t* camera)
{
    node_t* curNode = entityManager->entities->first;
    while (curNode != NULL)
    {
        dn_entity_t* entity = (dn_entity_t*)curNode->val;
        dn_drawEntity(entity, entityManager, camera);
        curNode = curNode->next;
    }
}

void dn_freeData(dn_entity_t* entity)
{
    if (entity == NULL) return;

    if (entity->data != NULL)
    {
        heap_caps_free(entity->data);
        entity->data = NULL;
    }
}

void dn_destroyAllEntities(dn_entityManager_t* entityManager)
{
    dn_entity_t* curEntity;
    while (NULL != (curEntity = pop(entityManager->entities)))
    {
        dn_freeData(curEntity);
        heap_caps_free(curEntity);
    }
}

dn_entity_t* dn_createEntity(dn_entityManager_t* entityManager, dn_animationType_t type, bool paused,
                             dn_spriteDef_t spriteIndex, uint8_t gameFramesPerAnimationFrame, uint32_t x, uint32_t y)
{
    dn_entity_t* entity = heap_caps_calloc(1, sizeof(dn_entity_t), MALLOC_CAP_SPIRAM);
    if (entity == NULL)
    {
        return NULL;
    }

    entity->active                     = true;
    entity->pos.x                      = x;
    entity->pos.y                      = y;

    push(entityManager->entities, (void*)entity);
    return entity;
}

void dn_freeEntityManager(dn_entityManager_t* entityManager)
{
    if (entityManager == NULL) return;

    dn_destroyAllEntities(entityManager);
    heap_caps_free(entityManager->entities);
    entityManager->entities = NULL;
}

