//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>

#include "gameData_bigbug.h"
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
    rocketSprite->originX        = 33;
    rocketSprite->originY        = 67;
    printf("rocket numFrames %d\n", entityManager->sprites[ROCKET_ANIM].numFrames);

    bb_sprite_t* flameSprite    = bb_loadSprite("flame", 24, &entityManager->sprites[FLAME_ANIM]);
    flameSprite->originX        = 26;
    flameSprite->originY        = -27;
    printf("flame numFrames %d\n", entityManager->sprites[FLAME_ANIM].numFrames);

    bb_sprite_t* garbotnikFlyingSprite = bb_loadSprite("garbotnik-", 3, &entityManager->sprites[GARBOTNIK_FLYING]);
    garbotnikFlyingSprite->originX = 18;
    garbotnikFlyingSprite->originY = 17;
    printf("flame numFrames %d\n", entityManager->sprites[GARBOTNIK_FLYING].numFrames);
}

void bb_updateEntities(bb_entityManager_t* entityManager, rectangle_t* camera)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        if (entityManager->entities[i].active)
        {
            if(entityManager->entities[i].updateFunction != NULL){
                entityManager->entities[i].updateFunction(&(entityManager->entities[i]));
            }
            if (&(entityManager->entities[i]) == entityManager->viewEntity)
            {
                bb_viewFollowEntity(&(entityManager->entities[i]), camera);
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
        bb_entity_t* currentEntity = &entityManager->entities[i];
        
        if (currentEntity->active)
        {
            // printf("hey %d\n", currentEntity->spriteIndex);
            if (currentEntity->drawFunction != NULL){
                currentEntity->drawFunction(entityManager, camera, &currentEntity);
            }
            else{
                drawWsgSimple(&entityManager->sprites[currentEntity->spriteIndex].frames[currentEntity->currentAnimationFrame],
                    (currentEntity->pos.x >> SUBPIXEL_RESOLUTION)
                        - entityManager->sprites[currentEntity->spriteIndex].originX - camera->pos.x,
                    (currentEntity->pos.y >> SUBPIXEL_RESOLUTION)
                        - entityManager->sprites[currentEntity->spriteIndex].originY - camera->pos.y);

                printf("sprite %d frame %d\n", currentEntity->spriteIndex, currentEntity->currentAnimationFrame);
            }
            

            if(currentEntity->paused == false){
                //increment the frame counter
                currentEntity->animationTimer += 1;
                currentEntity->currentAnimationFrame = currentEntity->animationTimer / currentEntity->gameFramesPerAnimationFrame;
                //if frame reached the end of the animation
                if (currentEntity->currentAnimationFrame
                    >= entityManager->sprites[currentEntity->spriteIndex].numFrames)
                {
                    switch (currentEntity->type)
                    {
                    case ONESHOT_ANIMATION:
                        //destroy the entity
                        bb_destroyEntity(&currentEntity, false);
                        break;
                    
                    case LOOPING_ANIMATION:
                        //reset the animation
                        currentEntity->animationTimer = 0;
                        currentEntity->currentAnimationFrame = 0;
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

void bb_viewFollowEntity(bb_entity_t* entity, rectangle_t* camera)
{
    // Update the camera's position to catch up to the player
    if (((entity->pos.x - HALF_WIDTH) >> DECIMAL_BITS) - camera->pos.x < -15)
    {
        camera->pos.x = ((entity->pos.x - HALF_WIDTH) >> DECIMAL_BITS) + 15;
    }
    else if (((entity->pos.x - HALF_WIDTH) >> DECIMAL_BITS) - camera->pos.x > 15)
    {
        camera->pos.x = ((entity->pos.x - HALF_WIDTH) >> DECIMAL_BITS) - 15;
    }

    if (((entity->pos.y - HALF_HEIGHT) >> DECIMAL_BITS) - camera->pos.y < -10)
    {
        camera->pos.y = ((entity->pos.y - HALF_HEIGHT) >> DECIMAL_BITS) + 10;
    }
    else if (((entity->pos.y - HALF_HEIGHT) >> DECIMAL_BITS) - camera->pos.y > 10)
    {
        camera->pos.y = ((entity->pos.y - HALF_HEIGHT) >> DECIMAL_BITS) - 10;
    }
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
            bb_garbotnikData* gData = calloc(1, sizeof(bb_garbotnikData));
            entity->data = gData;
            entity->cSquared = 73728;

            entity->updateFunction = &bb_updateGarbotnikFlying;
            entity->drawFunction   = &bb_drawGarbotnikFlying;

            //entityManager->viewEntity = entity;
            entityManager->playerEntity = entity;
            break;
        case ROCKET_ANIM:
            bb_rocketData_t* rData = calloc(1, sizeof(bb_rocketData_t));
            rData->flame = NULL;
            rData->yVel = 240;
            entity->data = rData;
            entity->cSquared = 243968;

            entity->updateFunction = &bb_updateRocketLanding;
            entityManager->viewEntity = entity;
            entityManager->playerEntity = entity;
            break;
        case FLAME_ANIM:
            entity->updateFunction = &bb_updateFlame;
            break;
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