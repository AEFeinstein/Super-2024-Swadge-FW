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
void bb_initializeEntityManager(bb_entityManager_t* entityManager, bb_gameData_t* gameData, bb_soundManager_t* soundManager)
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

bb_sprite_t* bb_loadSprite(char name[], bb_spriteDef_t def, uint8_t num_frames)
{
    bb_sprite_t* sprite = (bb_sprite_t*)malloc(num_frames * sizeof(wsg_t) + sizeof(bb_sprite_t));
    for(uint8_t i = 0; i < num_frames; i++)
    {
        char wsg_name[strlen(name)+7];//7 extra characters makes room for up to a 2 digit number + ".wsg" + null terminator ('\0')
        strcpy(wsg_name, name);
        //Code to cast an int to a string.
        uint8_t length = snprintf(NULL, 0, "%d", i);
        char* str = malloc(length+1);//+1 is to account for the null terminator ('\0')
        snprintf(str, length+1, "%d", i);
        strcat(wsg_name, str);
        free(str);
        strcat(wsg_name, ".wsg");//now name looks something like crumble12.wsg
        wsg_t* wsg = calloc(1, sizeof(wsg_t));
        loadWsg(wsg_name, wsg, true);

        sprite->frames[i] = wsg;
    }
    return sprite;
}

void bb_loadSprites(bb_entityManager_t* entityManager)
{
    bb_sprite_t* sprite = bb_loadSprite((char[13]){"crumble"}, CRUMBLE_ANIMATION, 24);
    sprite->originX = 0;
    sprite->originY = 0;
    entityManager->sprites[CRUMBLE_ANIMATION] = *sprite;
    free(sprite);

    // entityManager->sprites[CRUMBLE_ANIMATION] = calloc(1, sizeof(list_t));
    
    // for(uint8_t i = 1; i < 25; i++)//24 frames
    // {
    //     bb_sprite_t* sprite = malloc(sizeof(bb_sprite_t));
    //     sprite->originX = 0;
    //     sprite->originY = 0;
        
    //     //Code to cast an int to a string.
    //     uint8_t length = snprintf(NULL, 0, "%d", i);
    //     char* str = malloc(length+1);
    //     snprintf(str, length+1, "%d", i);

    //     char name[13] = "crumble";
    //     strcat(name, str);
    //     free(str);
    //     strcat(name, ".wsg");
    //     loadWsg(name, &(sprite->wsg), true);//what I actually wanted to do 2 hours ago.

    //     //push to tail
    //     push(entityManager->sprites[CRUMBLE_ANIMATION], (void*)sprite);
    // }
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
        if (!currentEntity->active)
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

        if (currentEntity.active)
        {
            drawWsg((wsg_t*)currentEntity.currentFrame->val,
                    (currentEntity.x >> SUBPIXEL_RESOLUTION) - currentEntity.sprite.originX,
                    (currentEntity.y >> SUBPIXEL_RESOLUTION) - currentEntity.sprite.originY,
                    currentEntity.spriteFlipHorizontal, currentEntity.spriteFlipVertical,
                    currentEntity.spriteRotateAngle);

            currentEntity.currentFrame =  currentEntity.currentFrame->next;
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
    free(self->sprites);
}