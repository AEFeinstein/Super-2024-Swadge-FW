//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>

#include "gameData_bigbug.h"
#include "entityManager_bigbug.h"
#include "entity_bigbug.h"
#include "lighting_bigbug.h"
#include "random_bigbug.h"
#include "aabb_utils_bigbug.h"

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
    entityManager->entities = heap_caps_calloc(MAX_ENTITIES, sizeof(bb_entity_t), MALLOC_CAP_SPIRAM);

    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_initializeEntity(&(entityManager->entities[i]), entityManager, gameData, soundManager);
    }

    entityManager->activeEntities = 0;

    // Use calloc to ensure members are all 0 or NULL
    entityManager->cachedEntities = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
}

bb_sprite_t* bb_loadSprite(const char name[], uint8_t num_frames, uint8_t brightnessLevels, bb_sprite_t* sprite)
{
    sprite->numFrames = num_frames;
    sprite->frames    = heap_caps_calloc(brightnessLevels * num_frames, sizeof(wsg_t), MALLOC_CAP_SPIRAM);

    for (uint8_t brightness = 0; brightness < brightnessLevels; brightness++)
    {
        for (uint8_t i = 0; i < num_frames; i++)
        {
            char wsg_name[strlen(name) + 8]; // 7 extra characters makes room for up to a 3 digit number + ".wsg" + null
                                             // terminator ('\0')
            snprintf(wsg_name, sizeof(wsg_name), "%s%d.wsg", name, brightness * num_frames + i);
            loadWsg(wsg_name, &sprite->frames[brightness * num_frames + i], true);
        }
    }

    return sprite;
}

void bb_loadSprites(bb_entityManager_t* entityManager)
{
    bb_sprite_t* crumbleSprite = bb_loadSprite("crumble", 24, 1, &entityManager->sprites[CRUMBLE_ANIM]);
    crumbleSprite->originX     = 48;
    crumbleSprite->originY     = 43;
    printf("crumble numFrames %d\n", entityManager->sprites[CRUMBLE_ANIM].numFrames);

    bb_sprite_t* bumpSprite = bb_loadSprite("hit", 8, 1, &entityManager->sprites[BUMP_ANIM]);
    bumpSprite->originX     = 37;
    bumpSprite->originY     = 37;
    printf("bump numFrames %d\n", entityManager->sprites[BUMP_ANIM].numFrames);

    bb_sprite_t* rocketSprite = bb_loadSprite("rocket", 41, 1, &entityManager->sprites[ROCKET_ANIM]);
    rocketSprite->originX     = 33;
    rocketSprite->originY     = 67;
    printf("rocket numFrames %d\n", entityManager->sprites[ROCKET_ANIM].numFrames);

    bb_sprite_t* flameSprite = bb_loadSprite("flame", 24, 1, &entityManager->sprites[FLAME_ANIM]);
    flameSprite->originX     = 26;
    flameSprite->originY     = -27;
    printf("flame numFrames %d\n", entityManager->sprites[FLAME_ANIM].numFrames);

    bb_sprite_t* garbotnikFlyingSprite = bb_loadSprite("garbotnik-", 3, 1, &entityManager->sprites[GARBOTNIK_FLYING]);
    garbotnikFlyingSprite->originX     = 18;
    garbotnikFlyingSprite->originY     = 17;
    printf("garbotnik numFrames %d\n", entityManager->sprites[GARBOTNIK_FLYING].numFrames);

    bb_sprite_t* harpoonSprite = bb_loadSprite("harpoon-", 18, 1, &entityManager->sprites[HARPOON]);
    harpoonSprite->originX     = 10;
    harpoonSprite->originY     = 10;
    printf("harpoon numFrames %d\n", entityManager->sprites[HARPOON].numFrames);

    bb_sprite_t* eggLeavesSprite = bb_loadSprite("eggLeaves", 1, 6, &entityManager->sprites[EGG_LEAVES]);
    eggLeavesSprite->originX     = 12;
    eggLeavesSprite->originY     = 5;
    printf("eggLeaves numFrames %d\n", entityManager->sprites[EGG_LEAVES].numFrames);

    bb_sprite_t* eggSprite = bb_loadSprite("egg", 1, 6, &entityManager->sprites[EGG]);
    eggSprite->originX     = 12;
    eggSprite->originY     = 12;
    printf("egg numFrames %d\n", entityManager->sprites[EGG].numFrames);

    bb_sprite_t* buSprite = bb_loadSprite("bu", 4, 6, &entityManager->sprites[BU]);
    buSprite->originX     = 13;
    buSprite->originY     = 15;
    printf("bu numFrames %d\n", entityManager->sprites[BU].numFrames);

    bb_sprite_t* bugSprite = bb_loadSprite("bug", 4, 6, &entityManager->sprites[BUG]);
    bugSprite->originX     = 13;
    bugSprite->originY     = 7;
    printf("bug numFrames %d\n", entityManager->sprites[BUG].numFrames);

    bb_sprite_t* buggSprite = bb_loadSprite("bugg", 4, 6, &entityManager->sprites[BUGG]);
    buggSprite->originX     = 11;
    buggSprite->originY     = 11;
    printf("bugg numFrames %d\n", entityManager->sprites[BUGG].numFrames);

    bb_sprite_t* buggoSprite = bb_loadSprite("buggo", 4, 6, &entityManager->sprites[BUGGO]);
    buggoSprite->originX     = 12;
    buggoSprite->originY     = 14;
    printf("buggo numFrames %d\n", entityManager->sprites[BUGGO].numFrames);

    bb_sprite_t* buggySprite = bb_loadSprite("buggy", 4, 6, &entityManager->sprites[BUGGY]);
    buggySprite->originX     = 13;
    buggySprite->originY     = 11;
    printf("buggy numFrames %d\n", entityManager->sprites[BUGGY].numFrames);

    bb_sprite_t* buttSprite = bb_loadSprite("butt", 4, 6, &entityManager->sprites[BUTT]);
    buttSprite->originX     = 14;
    buttSprite->originY     = 6;
    printf("butt numFrames %d\n", entityManager->sprites[BUTT].numFrames);
}

void bb_updateEntities(bb_entityManager_t* entityManager, rectangle_t* camera)
{
    vec_t shiftedCameraPos = camera->pos;
    shiftedCameraPos.x     = (shiftedCameraPos.x + 140) << DECIMAL_BITS;
    shiftedCameraPos.y     = (shiftedCameraPos.y + 120) << DECIMAL_BITS;
    node_t* currentNode    = entityManager->cachedEntities->first;
    // This loop loads entities back in if they are close to the camera.
    while (currentNode != NULL)
    {
        bb_entity_t* curEntity = (bb_entity_t*)currentNode->val;
        node_t* next           = currentNode->next;
        // Camera diagonal explained
        // 280 240 tft width x height
        // 140 120 halfWidth halfHeight
        // 2240 1920 bit shifted << 4
        // 5,017,600 3,686,400 squared
        // 8,704,000 added
        if (sqMagVec2d(subVec2d(curEntity->pos, shiftedCameraPos)) <= curEntity->cSquared + 8704000)
        { // if it is close
            bb_entity_t* foundSpot = bb_findInactiveEntity(entityManager);
            if (foundSpot != NULL)
            {
                // like a memcopy
                *foundSpot = *curEntity;
                entityManager->activeEntities++;
                removeEntry(entityManager->cachedEntities, currentNode);
            }
        }
        currentNode = next;
    }

    // This loops over all entities, doing updates and collision checks and moving the camera to the viewEntity.
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_entity_t* curEntity = &entityManager->entities[i];
        if (curEntity->active)
        {
            if (curEntity->cacheable)
            {
                if (sqMagVec2d(subVec2d(curEntity->pos, shiftedCameraPos)) > curEntity->cSquared + 8704000)
                { // if it is far
                    // This entity gets cached
                    bb_entity_t* cachedEntity = heap_caps_malloc(sizeof(bb_entity_t), MALLOC_CAP_SPIRAM);
                    // It's like a memcopy
                    *cachedEntity = *curEntity;
                    // push to the tail
                    push(entityManager->cachedEntities, (void*)cachedEntity);
                    bb_destroyEntity(curEntity);
                    continue;
                }
            }

            if (curEntity->updateFunction != NULL)
            {
                curEntity->updateFunction(&(entityManager->entities[i]));
            }
            if (curEntity->updateFarFunction != NULL)
            {
                if (bb_boxesCollideShift(&(bb_box_t){addVec2d(camera->pos, (vec_t){140, 120}), 140, 120},
                                         &(bb_box_t){curEntity->pos, curEntity->halfWidth, curEntity->halfHeight})
                    == false)
                {
                    curEntity->updateFarFunction(curEntity);
                }
            }

            if (curEntity->collisions != NULL)
            {
                for (uint8_t j = 0; j < MAX_ENTITIES; j++)
                {
                    bb_entity_t* collisionCandidate = &entityManager->entities[j];
                    // Iterate over all nodes
                    node_t* currentCollisionCheck = curEntity->collisions->first;
                    while (currentCollisionCheck != NULL)
                    {
                        node_t* currentOtherType = ((bb_collision_t*)currentCollisionCheck->val)->checkOthers->first;
                        while (currentOtherType != NULL)
                        {
                            if (collisionCandidate->spriteIndex == *((bb_spriteDef_t*)currentOtherType->val))
                            {
                                //do a collision check here
                                if (bb_boxesCollide(&(bb_box_t){collisionCandidate->pos, collisionCandidate->halfWidth, collisionCandidate->halfHeight},
                                         &(bb_box_t){curEntity->pos, curEntity->halfWidth, curEntity->halfHeight}))
                                {
                                    ((bb_collision_t*)currentCollisionCheck->val)->function(curEntity, collisionCandidate);
                                }
                                break;
                            }
                            currentOtherType = currentOtherType->next;
                            if (curEntity->collisions == NULL)
                            {
                                break;
                            }
                        }
                        currentCollisionCheck = currentCollisionCheck->next;
                        if (curEntity->collisions == NULL)
                        {
                            break;
                        }
                    }
                    if (curEntity->collisions == NULL)
                    {
                        break;
                    }
                }
            }

            if (curEntity == entityManager->viewEntity)
            {
                bb_viewFollowEntity(curEntity, camera);
            }
        }
    }
}

void bb_deactivateAllEntities(bb_entityManager_t* entityManager, bool excludePlayer)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_entity_t* currentEntity = &(entityManager->entities[i]);
        if (!currentEntity->active)
        {
            continue;
        }

        bb_destroyEntity(currentEntity);

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
            // printf("hey %d %d\n", i, currentEntity->spriteIndex);
            if (currentEntity->drawFunction != NULL)
            {
                currentEntity->drawFunction(entityManager, camera, currentEntity);
            }
            else if (currentEntity->hasLighting)
            {
                vec_t lookup = {
                    .x = (currentEntity->pos.x >> DECIMAL_BITS) - (entityManager->playerEntity->pos.x >> DECIMAL_BITS)
                         + currentEntity->gameData->tilemap.headlampWsg.w,
                    .y = (currentEntity->pos.y >> DECIMAL_BITS) - (entityManager->playerEntity->pos.y >> DECIMAL_BITS)
                         + currentEntity->gameData->tilemap.headlampWsg.h};

                lookup = divVec2d(lookup, 2);

                int16_t xOff = (currentEntity->pos.x >> DECIMAL_BITS)
                               - entityManager->sprites[currentEntity->spriteIndex].originX - camera->pos.x;
                int16_t yOff = (currentEntity->pos.y >> DECIMAL_BITS)
                               - entityManager->sprites[currentEntity->spriteIndex].originY - camera->pos.y;

                uint8_t brightness = 5;
                if (currentEntity->pos.y > 5120)
                {
                    if (currentEntity->pos.y > 30720)
                    {
                        brightness = 0;
                    }
                    else
                    {
                        brightness = (30720 - currentEntity->pos.y) / 5120;
                    }
                }

                if (currentEntity->gameData->entityManager.playerEntity == NULL)
                {
                    drawWsgSimple(&entityManager->sprites[currentEntity->spriteIndex]
                                       .frames[brightness + currentEntity->currentAnimationFrame * 6],
                                  xOff, yOff);
                }
                else
                {
                    drawWsgSimple(
                        &entityManager->sprites[currentEntity->spriteIndex]
                             .frames[bb_midgroundLighting(&(currentEntity->gameData->tilemap.headlampWsg), &lookup,
                                                          &(((bb_garbotnikData_t*)currentEntity->gameData->entityManager
                                                                 .playerEntity->data)
                                                                ->yaw.x),
                                                          brightness)
                                     + currentEntity->currentAnimationFrame * 6],
                        xOff, yOff);
                }
            }
            else
            {
                drawWsgSimple(
                    &entityManager->sprites[currentEntity->spriteIndex].frames[currentEntity->currentAnimationFrame],
                    (currentEntity->pos.x >> SUBPIXEL_RESOLUTION)
                        - entityManager->sprites[currentEntity->spriteIndex].originX - camera->pos.x,
                    (currentEntity->pos.y >> SUBPIXEL_RESOLUTION)
                        - entityManager->sprites[currentEntity->spriteIndex].originY - camera->pos.y);
            }

            if (currentEntity->paused == false)
            {
                // increment the frame counter
                currentEntity->animationTimer += 1;
                currentEntity->currentAnimationFrame
                    = currentEntity->animationTimer / currentEntity->gameFramesPerAnimationFrame;
                // if frame reached the end of the animation
                if (currentEntity->currentAnimationFrame
                    >= entityManager->sprites[currentEntity->spriteIndex].numFrames)
                {
                    switch (currentEntity->type)
                    {
                        case ONESHOT_ANIMATION:
                        {
                            // destroy the entity
                            bb_destroyEntity(currentEntity);
                            break;
                        }
                        case LOOPING_ANIMATION:
                        {
                            // reset the animation
                            currentEntity->animationTimer        = 0;
                            currentEntity->currentAnimationFrame = 0;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }
            }
            drawRect (((currentEntity->pos.x - currentEntity->halfWidth) >>DECIMAL_BITS) - camera->pos.x,
                      ((currentEntity->pos.y - currentEntity->halfHeight)>>DECIMAL_BITS) - camera->pos.y,
                      ((currentEntity->pos.x + currentEntity->halfWidth) >>DECIMAL_BITS) - camera->pos.x,
                      ((currentEntity->pos.y + currentEntity->halfHeight)>>DECIMAL_BITS) - camera->pos.y, c500);
        }
    }
}

bb_entity_t* bb_findInactiveEntity(bb_entityManager_t* entityManager)
{
    if (entityManager->activeEntities == MAX_ENTITIES)
    {
        return NULL;
    };

    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        if (entityManager->entities[i].active == false)
        {
            return &(entityManager->entities[i]);
        }
    }
    return NULL;
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

bb_entity_t* bb_createEntity(bb_entityManager_t* entityManager, bb_animationType_t type, bool paused,
                             bb_spriteDef_t spriteIndex, uint8_t gameFramesPerAnimationFrame, uint32_t x, uint32_t y)
{
    if (entityManager->activeEntities == MAX_ENTITIES)
    {
        // printf("Failed entity creation. MAX_ENTITIES exceeded.\n");
        return NULL;
    }

    bb_entity_t* entity = bb_findInactiveEntity(entityManager);

    if (spriteIndex == GARBOTNIK_FLYING)
    {
        // Just forcibly make garbotnik the last entity so he's drawn on top.
        entity = &entityManager->entities[MAX_ENTITIES - 1];
    }

    if (entity == NULL)
    {
        printf("entityManager_bigbug.c This should hopefully never happen.\n");
        return NULL;
    }

    entity->active = true;
    entity->pos.x = x << SUBPIXEL_RESOLUTION, entity->pos.y = y << SUBPIXEL_RESOLUTION;

    entity->type        = type;
    entity->paused      = paused;
    entity->spriteIndex = spriteIndex;

    entity->gameFramesPerAnimationFrame = gameFramesPerAnimationFrame;
    // entity->collisionHandler     = &dummyCollisionHandler;
    // entity->tileCollisionHandler = &ballTileCollisionHandler;

    switch (spriteIndex)
    {
        case GARBOTNIK_FLYING:
        {
            bb_garbotnikData_t* gData = heap_caps_calloc(1, sizeof(bb_garbotnikData_t), MALLOC_CAP_SPIRAM);
            gData->numHarpoons        = 100;
            gData->fuel  = 1000 * 60 * 1; // 1 thousand milliseconds in a second. 60 seconds in a minute. 1 minutes.
            entity->data = gData;

            entity->halfWidth  = 192;
            entity->halfHeight = 192;

            entity->updateFunction = &bb_updateGarbotnikFlying;
            entity->drawFunction   = &bb_drawGarbotnikFlying;

            // entityManager->viewEntity = entity;
            entityManager->playerEntity = entity;
            break;
        }
        case ROCKET_ANIM:
        {
            bb_rocketData_t* rData = heap_caps_calloc(1, sizeof(bb_rocketData_t), MALLOC_CAP_SPIRAM);
            rData->flame           = NULL;
            rData->yVel            = 240;
            entity->data           = rData;

            entity->halfWidth  = 192;
            entity->halfHeight = 464;

            entity->updateFunction    = &bb_updateRocketLanding;
            entityManager->viewEntity = entity;
            break;
        }
        case HARPOON:
        {
            bb_projectileData_t* pData = heap_caps_calloc(1, sizeof(bb_projectileData_t), MALLOC_CAP_SPIRAM);
            entity->data               = pData;

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);

            bb_spriteDef_t* bu = heap_caps_malloc(sizeof(bb_spriteDef_t), MALLOC_CAP_SPIRAM);
            *bu                = BU;
            push(others, (void*)bu);

            bb_spriteDef_t* bug = heap_caps_malloc(sizeof(bb_spriteDef_t), MALLOC_CAP_SPIRAM);
            *bug                = BUG;
            push(others, (void*)bug);

            bb_spriteDef_t* bugg = heap_caps_malloc(sizeof(bb_spriteDef_t), MALLOC_CAP_SPIRAM);
            *bugg                = BUGG;
            push(others, (void*)bugg);

            bb_spriteDef_t* buggo = heap_caps_malloc(sizeof(bb_spriteDef_t), MALLOC_CAP_SPIRAM);
            *buggo                = BUGGO;
            push(others, (void*)buggo);

            bb_spriteDef_t* buggy = heap_caps_malloc(sizeof(bb_spriteDef_t), MALLOC_CAP_SPIRAM);
            *buggy                = BUGGY;
            push(others, (void*)buggy);

            bb_spriteDef_t* butt = heap_caps_malloc(sizeof(bb_spriteDef_t), MALLOC_CAP_SPIRAM);
            *butt                = BUTT;
            push(others, (void*)butt);

            bb_collision_t* collision = heap_caps_malloc(sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionHarpoon};
            push(entity->collisions, (void*)collision);

            entity->updateFunction = &bb_updateHarpoon;
            entity->drawFunction   = &bb_drawHarpoon;
            break;
        }
        case EGG_LEAVES:
        {
            bb_eggLeavesData_t* elData = heap_caps_calloc(1, sizeof(bb_eggLeavesData_t), MALLOC_CAP_SPIRAM);
            entity->data               = elData;

            entity->updateFunction    = &bb_updateEggLeaves;
            entity->updateFarFunction = &bb_updateFarEggleaves;
            entity->drawFunction      = &bb_drawEggLeaves;
            break;
        }
        case EGG:
        {
            bb_eggData_t* eData = heap_caps_calloc(1, sizeof(bb_eggData_t), MALLOC_CAP_SPIRAM);
            entity->data        = eData;

            entity->drawFunction = &bb_drawEgg;
            break;
        }
        case BU:
        {
            bb_bugData_t* bData = heap_caps_calloc(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health = 100;
            entity->data        = bData;

            entity->hasLighting                 = true;
            entity->gameFramesPerAnimationFrame = bb_randomInt(2, 4);

            entity->cacheable = true;

            entity->halfWidth  = 192;
            entity->halfHeight = 104;

            entity->updateFunction = &bb_updateBug;
            break;
        }
        case BUG:
        {
            bb_bugData_t* bData = heap_caps_calloc(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health = 100;
            entity->data        = bData;

            entity->hasLighting                 = true;
            entity->gameFramesPerAnimationFrame = bb_randomInt(2, 4);

            entity->cacheable = true;

            entity->halfWidth  = 176;
            entity->halfHeight = 48;

            entity->updateFunction = &bb_updateBug;
            break;
        }
        case BUGG:
        {
            bb_bugData_t* bData = heap_caps_calloc(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health = 100;
            entity->data        = bData;

            entity->hasLighting                 = true;
            entity->gameFramesPerAnimationFrame = bb_randomInt(2, 4);

            entity->cacheable = true;

            entity->halfWidth  = 120;
            entity->halfHeight = 104;

            entity->updateFunction = &bb_updateBug;
            break;
        }
        case BUGGO:
        {
            bb_bugData_t* bData = heap_caps_calloc(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health = 100;
            entity->data        = bData;

            entity->hasLighting                 = true;
            entity->gameFramesPerAnimationFrame = bb_randomInt(2, 4);

            entity->cacheable = true;

            entity->halfWidth  = 144;
            entity->halfHeight = 144;

            entity->updateFunction = &bb_updateBug;
            break;
        }
        case BUGGY:
        {
            bb_bugData_t* bData = heap_caps_calloc(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health = 100;
            entity->data        = bData;

            entity->hasLighting                 = true;
            entity->gameFramesPerAnimationFrame = bb_randomInt(2, 4);

            entity->cacheable = true;

            entity->halfWidth  = 184;
            entity->halfHeight = 64;

            entity->updateFunction = &bb_updateBug;
            break;
        }
        case BUTT:
        {
            bb_bugData_t* bData = heap_caps_calloc(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health = 100;
            entity->data        = bData;

            entity->hasLighting                 = true;
            entity->gameFramesPerAnimationFrame = bb_randomInt(2, 4);

            entity->cacheable = true;

            entity->halfWidth  = 184;
            entity->halfHeight = 88;

            entity->updateFunction = &bb_updateBug;
            break;
        }
        default: // FLAME_ANIM and others need nothing set
        {
            break;
        }
    }
    entity->cSquared = entity->halfWidth * entity->halfWidth + entity->halfHeight * entity->halfHeight;

    entityManager->activeEntities++;
    // if(entityManager->activeEntities % 10 == 0 || entityManager->activeEntities == MAX_ENTITIES){
    //     printf("%d/%d entities ^\n", entityManager->activeEntities, MAX_ENTITIES);
    // }

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
    // free and clear
    free(self->entities);
    clear(self->cachedEntities);
}