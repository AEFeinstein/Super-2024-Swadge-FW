//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>

#include "entityManager_bigbug.h"
#include "entity_bigbug.h"

#include "esp_random.h"
#include "palette.h"

#include "fs_wsg.h"

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

bb_sprite_t* bb_loadSprite(const char name[], uint8_t num_frames, bb_sprite_t* sprite)
{
    sprite->numFrames = num_frames;
    sprite->frames    = malloc(sizeof(wsg_t) * num_frames);
    for (uint8_t i = 0; i < num_frames; i++)
    {
        char wsg_name[strlen(name) + 7]; // 7 extra characters makes room for up to a 2 digit number + ".wsg" + null
                                         // terminator ('\0')
        snprintf(wsg_name, sizeof(wsg_name), "%s%d.wsg", name, i);
        loadWsg(wsg_name, &sprite->frames[i], true);
    }

    return sprite;
}

void bb_loadSprites(bb_entityManager_t* entityManager)
{
    bb_sprite_t* crumbleSprite = bb_loadSprite("crumble", 24, &entityManager->sprites[CRUMBLE_ANIM]);
    crumbleSprite->originX     = 48;
    crumbleSprite->originY     = 43;
    printf("crumble numFrames %d\n", entityManager->sprites[CRUMBLE_ANIM].numFrames);

    bb_sprite_t* bumpSprite    = bb_loadSprite("hit", 8, &entityManager->sprites[BUMP_ANIM]);
    bumpSprite->originX        = 37;
    bumpSprite->originY        = 37;
    printf("bump numFrames %d\n", entityManager->sprites[BUMP_ANIM].numFrames);

    bb_sprite_t* rocketSprite    = bb_loadSprite("rocket", 35, &entityManager->sprites[ROCKET_ANIM]);
    rocketSprite->originX        = 32;
    rocketSprite->originY        = 28;
    printf("rocket numFrames %d\n", entityManager->sprites[ROCKET_ANIM].numFrames);

    bb_sprite_t* flameSprite    = bb_loadSprite("flame", 24, &entityManager->sprites[FLAME_ANIM]);
    flameSprite->originX        = 26;
    flameSprite->originY        = -66;
    printf("flame numFrames %d\n", entityManager->sprites[FLAME_ANIM].numFrames);

    bb_sprite_t* garbotnikFlyingSprite = bb_loadSprite("garbotnik-", 3, &entityManager->sprites[GARBOTNIK_FLYING]);
    garbotnikFlyingSprite->originX = 18;
    garbotnikFlyingSprite->originY = 17;
    printf("flame numFrames %d\n", entityManager->sprites[GARBOTNIK_FLYING].numFrames);
    // free(sprite);

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

void bb_updateEntities(bb_entityManager_t* entityManager, bb_gameData_t* gameData)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        if (entityManager->entities[i].active)
        {
            if(entityManager->entities[i].updateFunction != NULL){
                entityManager->entities[i].updateFunction(&(entityManager->entities[i]), gameData);
                if (&(entityManager->entities[i]) == entityManager->viewEntity)
                {
                    bb_viewFollowEntity(&(entityManager->entities[i]));
                }
            }
        }
    }
}

void bb_deactivateAllEntities(bb_entityManager_t* entityManager, bool excludePlayer, bool excludePersistent,
                              bool respawn)
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

void bb_drawEntities(bb_entityManager_t* entityManager, rectangle_t* camera)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_entity_t currentEntity = entityManager->entities[i];

        if (currentEntity.active)
        {
            // printf("hey %d\n", currentEntity.spriteIndex);

            drawWsgSimpleScaled(&entityManager->sprites[currentEntity.spriteIndex].frames[currentEntity.currentAnimationFrame],
                                (currentEntity.pos.x >> SUBPIXEL_RESOLUTION)
                                    - entityManager->sprites[currentEntity.spriteIndex].originX - camera->pos.x,
                                (currentEntity.pos.y >> SUBPIXEL_RESOLUTION)
                                    - entityManager->sprites[currentEntity.spriteIndex].originY - camera->pos.y,
                                1, 1);

            if(entityManager->entities[i].paused == false){
                //increment the frame counter
                entityManager->entities[i].animationTimer += 1;
                entityManager->entities[i].currentAnimationFrame = entityManager->entities[i].animationTimer / entityManager->entities[i].gameFramesPerAnimationFrame;
                //if frame reached the end of the animation
                if (entityManager->entities[i].currentAnimationFrame
                    >= entityManager->sprites[entityManager->entities[i].spriteIndex].numFrames)
                {
                    switch (entityManager->entities[i].type)
                    {
                    case ONESHOT_ANIMATION:
                        //destroy the entity
                        bb_destroyEntity(&entityManager->entities[i], false);
                        break;
                    
                    case LOOPING_ANIMATION:
                        //reset the animation
                        entityManager->entities[i].animationTimer = 0;
                        entityManager->entities[i].currentAnimationFrame = 0;
                        break;

                    default:
                        break;
                    }
                }
            }
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
    // int16_t moveViewByX = (entity->x) >> SUBPIXEL_RESOLUTION;
    // int16_t moveViewByY = (entity->y > 63616) ? 0 : (entity->y) >> SUBPIXEL_RESOLUTION;
}

bb_entity_t* bb_createEntity(bb_entityManager_t* entityManager, bb_animationType_t type, bool paused, bb_spriteDef_t spriteIndex,
                            uint8_t gameFramesPerAnimationFrame, uint32_t x, uint32_t y)
{
    if (entityManager->activeEntities == MAX_ENTITIES)
    {
        printf("OH NO!\n");
        return NULL;
    }

    bb_entity_t* entity = bb_findInactiveEntity(entityManager);

    if (entity == NULL)
    {
        printf("OH CRAP\n");
        return NULL;
    }

    entity->active = true;
    entity->pos.x = x << SUBPIXEL_RESOLUTION,
    entity->pos.y = y << SUBPIXEL_RESOLUTION;

    entity->type        = type;
    entity->paused      = paused;
    entity->spriteIndex = spriteIndex;

    entity->animationTimer = 0;
    entity->gameFramesPerAnimationFrame = gameFramesPerAnimationFrame;
    entity->currentAnimationFrame = 0;
    // entity->collisionHandler     = &dummyCollisionHandler;
    // entity->tileCollisionHandler = &ballTileCollisionHandler;

    switch (spriteIndex)
    {
        case GARBOTNIK_FLYING:
            bb_garbotnikData* data = malloc(sizeof(bb_garbotnikData));
            entity->data = data;

            entity->updateFunction = &bb_updateGarbotnikFlying;
    }

    if (entity != NULL)
    {
        entityManager->activeEntities++;
    }

    return entity;
}

void bb_freeEntityManager(bb_entityManager_t* self)
{


    for (uint8_t i = 0; i < NUM_SPRITES; i++)
    {
        for (uint8_t f = 0; f < self->sprites[i].numFrames; f++)
        {
            freeWsg(&self->sprites[i].frames[f]);
        }
    }
    free(self->entities);
}