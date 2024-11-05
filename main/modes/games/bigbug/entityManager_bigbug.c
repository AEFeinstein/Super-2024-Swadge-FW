//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>

#include "gameData_bigbug.h"
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
    entityManager->entities = HEAP_CAPS_CALLOC_DBG(MAX_ENTITIES, sizeof(bb_entity_t), MALLOC_CAP_SPIRAM);

    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_initializeEntity(&(entityManager->entities[i]), entityManager, gameData, soundManager);
    }

    entityManager->activeEntities = 0;

    // Use calloc to ensure members are all 0 or NULL
    entityManager->cachedEntities = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
}

bb_sprite_t* bb_loadSprite(const char name[], uint8_t num_frames, uint8_t brightnessLevels, bb_sprite_t* sprite)
{
    sprite->brightnessLevels = brightnessLevels;
    sprite->numFrames = num_frames;
    sprite->frames    = HEAP_CAPS_CALLOC_DBG(brightnessLevels * num_frames, sizeof(wsg_t), MALLOC_CAP_SPIRAM);

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

    bb_sprite_t* bumpSprite = bb_loadSprite("hit", 8, 1, &entityManager->sprites[BUMP_ANIM]);
    bumpSprite->originX     = 37;
    bumpSprite->originY     = 37;

    bb_sprite_t* rocketSprite = bb_loadSprite("rocket", 41, 1, &entityManager->sprites[ROCKET_ANIM]);
    rocketSprite->originX     = 33;
    rocketSprite->originY     = 66;

    bb_sprite_t* flameSprite = bb_loadSprite("flame", 24, 1, &entityManager->sprites[FLAME_ANIM]);
    flameSprite->originX     = 27;
    flameSprite->originY     = -27;

    bb_sprite_t* garbotnikFlyingSprite = bb_loadSprite("garbotnik-", 3, 1, &entityManager->sprites[GARBOTNIK_FLYING]);
    garbotnikFlyingSprite->originX     = 18;
    garbotnikFlyingSprite->originY     = 17;

    bb_sprite_t* harpoonSprite = bb_loadSprite("harpoon-", 18, 1, &entityManager->sprites[HARPOON]);
    harpoonSprite->originX     = 10;
    harpoonSprite->originY     = 10;

    bb_sprite_t* eggLeavesSprite = bb_loadSprite("eggLeaves", 1, 6, &entityManager->sprites[EGG_LEAVES]);
    eggLeavesSprite->originX     = 12;
    eggLeavesSprite->originY     = 5;

    bb_sprite_t* eggSprite = bb_loadSprite("egg", 1, 6, &entityManager->sprites[EGG]);
    eggSprite->originX     = 12;
    eggSprite->originY     = 12;

    bb_sprite_t* buSprite = bb_loadSprite("bu", 4, 6, &entityManager->sprites[BU]);
    buSprite->originX     = 13;
    buSprite->originY     = 15;

    bb_sprite_t* bugSprite = bb_loadSprite("bug", 4, 6, &entityManager->sprites[BUG]);
    bugSprite->originX     = 13;
    bugSprite->originY     = 7;

    bb_sprite_t* buggSprite = bb_loadSprite("bugg", 4, 6, &entityManager->sprites[BUGG]);
    buggSprite->originX     = 11;
    buggSprite->originY     = 11;

    bb_sprite_t* buggoSprite = bb_loadSprite("buggo", 4, 6, &entityManager->sprites[BUGGO]);
    buggoSprite->originX     = 12;
    buggoSprite->originY     = 14;

    bb_sprite_t* buggySprite = bb_loadSprite("buggy", 4, 6, &entityManager->sprites[BUGGY]);
    buggySprite->originX     = 13;
    buggySprite->originY     = 11;

    bb_sprite_t* buttSprite = bb_loadSprite("butt", 4, 6, &entityManager->sprites[BUTT]);
    buttSprite->originX     = 14;
    buttSprite->originY     = 6;

    bb_sprite_t* menuSprite = bb_loadSprite("bb_menu", 4, 1, &entityManager->sprites[BB_MENU]);
    menuSprite->originX     = 140;
    menuSprite->originY     = 354;

    bb_sprite_t* deathDumpsterSprite = bb_loadSprite("DeathDumpster", 1, 1, &entityManager->sprites[BB_DEATH_DUMPSTER]);
    deathDumpsterSprite->originX     = 138;
    deathDumpsterSprite->originY     = 100;

    bb_sprite_t* attachmentArmSprite = bb_loadSprite("AttachmentArm", 1, 1, &entityManager->sprites[ATTACHMENT_ARM]);
    attachmentArmSprite->originX     = 3;
    attachmentArmSprite->originY     = 21;

    bb_loadSprite("ovo_talk", 8, 1, &entityManager->sprites[OVO_TALK]);
}

void bb_updateEntities(bb_entityManager_t* entityManager, bb_camera_t* camera)
{
    vec_t shiftedCameraPos = camera->camera.pos;
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
                    bb_entity_t* cachedEntity = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_entity_t), MALLOC_CAP_SPIRAM);
                    // It's like a memcopy
                    *cachedEntity = *curEntity;
                    // push to the tail
                    push(entityManager->cachedEntities, (void*)cachedEntity);
                    bb_destroyEntity(curEntity, true);
                    continue;
                }
            }

            if (curEntity->updateFunction != NULL)
            {
                curEntity->updateFunction(&(entityManager->entities[i]));
            }
            if (curEntity->updateFarFunction != NULL)
            {
                // 2752 = (140+32) << 4; 2432 = (120+32) << 4
                if (bb_boxesCollide(&(bb_entity_t){.pos = shiftedCameraPos, .halfWidth = 2752, .halfHeight = 2432},
                                    curEntity, NULL, NULL)
                    == false)
                {
                    curEntity->updateFarFunction(curEntity);
                }
            }

            if (curEntity->collisions != NULL)
            {
                if ((bb_spriteDef_t)(((bb_collision_t*)curEntity->collisions->first->val)->checkOthers->first)->val
                    == GARBOTNIK_FLYING)
                {
                    // no need to search all other entities if it's simply something to do with the player.
                    if (entityManager->playerEntity != NULL)
                    {
                        // do a collision check here
                        bb_hitInfo_t hitInfo = {0};
                        if (bb_boxesCollide(curEntity, entityManager->playerEntity,
                                            &(((bb_garbotnikData_t*)entityManager->playerEntity->data)->previousPos),
                                            &hitInfo))
                        {
                            ((bb_collision_t*)curEntity->collisions->first->val)
                                ->function(curEntity, entityManager->playerEntity, &hitInfo);
                        }
                    }
                }
                else
                {
                    for (uint8_t j = 0; j < MAX_ENTITIES; j++)
                    {
                        bb_entity_t* collisionCandidate = &entityManager->entities[j];
                        // Iterate over all nodes
                        node_t* currentCollisionCheck = curEntity->collisions->first;
                        while (currentCollisionCheck != NULL)
                        {
                            node_t* currentOtherType
                                = ((bb_collision_t*)currentCollisionCheck->val)->checkOthers->first;
                            node_t* cccNext = currentCollisionCheck->next;
                            while (currentOtherType != NULL)
                            {
                                if (collisionCandidate->spriteIndex == (bb_spriteDef_t)currentOtherType->val)
                                {
                                    // do a collision check here
                                    if (bb_boxesCollide(collisionCandidate, curEntity, NULL, NULL))
                                    {
                                        ((bb_collision_t*)currentCollisionCheck->val)
                                            ->function(curEntity, collisionCandidate, NULL);
                                    }
                                    break;
                                }
                                currentOtherType = currentOtherType->next;
                                if (curEntity->collisions == NULL)
                                {
                                    break;
                                }
                            }
                            currentCollisionCheck = cccNext;
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
            }

            if (curEntity == entityManager->viewEntity)
            {
                bb_viewFollowEntity(curEntity, camera);
            }

            bb_updateStarField(entityManager, camera);
        }
    }
}

void bb_updateStarField(bb_entityManager_t* entityManager, bb_camera_t* camera)
{
    if (camera->camera.pos.y < -600 && camera->camera.pos.y > -5390)
    {
        int16_t halfWidth  = HALF_WIDTH >> DECIMAL_BITS;
        int16_t halfHeight = HALF_HEIGHT >> DECIMAL_BITS;

        int skewedChance = 0;
        if(camera->camera.pos.y > -2000)
        {
            skewedChance = 128*camera->camera.pos.y + 256000;
        }

        if (bb_randomInt(1, 13000+skewedChance) < (abs(camera->velocity.x) * camera->camera.height))
        {
            bb_createEntity(entityManager, NO_ANIMATION, true, NO_SPRITE_STAR, 1,
                            camera->velocity.x > 0 ? camera->camera.pos.x + 2 * halfWidth : camera->camera.pos.x,
                            camera->camera.pos.y + halfHeight + bb_randomInt(-halfHeight, halfHeight), false);
        }
        if (bb_randomInt(1, 13000+skewedChance) < (abs(camera->velocity.y) * camera->camera.width))
        {
            bb_createEntity(entityManager, NO_ANIMATION, true, NO_SPRITE_STAR, 1,
                            camera->camera.pos.x + halfWidth + bb_randomInt(-halfWidth, halfWidth),
                            camera->velocity.y > 0 ? camera->camera.pos.y + 2 * halfHeight : camera->camera.pos.y,
                            false);
        }
        camera->velocity.x = 0;
        camera->velocity.y = 0;
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

        bb_destroyEntity(currentEntity, false);

        if (excludePlayer && currentEntity == entityManager->playerEntity)
        {
            currentEntity->active = true;
        }
    }

    // load all cached enemies and destroy them one by one.
    bb_entity_t* curEntity;
    while(NULL != (curEntity = pop(entityManager->cachedEntities)))
    {
        bb_destroyEntity(curEntity, false);
        FREE_DBG(curEntity);
    }
}

void bb_drawEntities(bb_entityManager_t* entityManager, rectangle_t* camera)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_entity_t* currentEntity = &entityManager->entities[i];

        if (currentEntity->active)
        {
            // if(currentEntity->spriteIndex == ATTACHMENT_ARM)
            // {
            //     printf("attachment arm x %d y %d\n", currentEntity->pos.x, currentEntity->pos.y);
            // }
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
                            bb_destroyEntity(currentEntity, false);
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
            // drawRect (((currentEntity->pos.x - currentEntity->halfWidth) >>DECIMAL_BITS) - camera->pos.x,
            //           ((currentEntity->pos.y - currentEntity->halfHeight)>>DECIMAL_BITS) - camera->pos.y,
            //           ((currentEntity->pos.x + currentEntity->halfWidth) >>DECIMAL_BITS) - camera->pos.x,
            //           ((currentEntity->pos.y + currentEntity->halfHeight)>>DECIMAL_BITS) - camera->pos.y, c500);
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

bb_entity_t* bb_findInactiveEntityBackwards(bb_entityManager_t* entityManager)
{
    if (entityManager->activeEntities == MAX_ENTITIES)
    {
        return NULL;
    };

    for (int i = MAX_ENTITIES - 1; i >= 0; i--)
    {
        if (entityManager->entities[i].active == false)
        {
            return &(entityManager->entities[i]);
        }
    }
    return NULL;
}

void bb_viewFollowEntity(bb_entity_t* entity, bb_camera_t* camera)
{
    vec_t previousPos = camera->camera.pos;
    // Update the camera's position to catch up to the player
    if (((entity->pos.x - HALF_WIDTH) >> DECIMAL_BITS) - camera->camera.pos.x < -15)
    {
        camera->camera.pos.x = ((entity->pos.x - HALF_WIDTH) >> DECIMAL_BITS) + 15;
    }
    else if (((entity->pos.x - HALF_WIDTH) >> DECIMAL_BITS) - camera->camera.pos.x > 15)
    {
        camera->camera.pos.x = ((entity->pos.x - HALF_WIDTH) >> DECIMAL_BITS) - 15;
    }
    if (((entity->pos.y - HALF_HEIGHT) >> DECIMAL_BITS) - camera->camera.pos.y < -10)
    {
        camera->camera.pos.y = ((entity->pos.y - HALF_HEIGHT) >> DECIMAL_BITS) + 10;
    }
    else if (((entity->pos.y - HALF_HEIGHT) >> DECIMAL_BITS) - camera->camera.pos.y > 10)
    {
        camera->camera.pos.y = ((entity->pos.y - HALF_HEIGHT) >> DECIMAL_BITS) - 10;
    }
    camera->velocity = addVec2d(camera->velocity,subVec2d(camera->camera.pos, previousPos));//velocity is this position minus previous position.
}

bb_entity_t* bb_createEntity(bb_entityManager_t* entityManager, bb_animationType_t type, bool paused,
                             bb_spriteDef_t spriteIndex, uint8_t gameFramesPerAnimationFrame, uint32_t x, uint32_t y,
                             bool renderFront)
{
    if (entityManager->activeEntities == MAX_ENTITIES)
    {
        // printf("Failed entity creation. MAX_ENTITIES exceeded.\n");
        return NULL;
    }

    bb_entity_t* entity;
    if (renderFront)
    {
        entity = bb_findInactiveEntityBackwards(entityManager);
    }
    else
    {
        entity = bb_findInactiveEntity(entityManager);
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
            bb_garbotnikData_t* gData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_garbotnikData_t), MALLOC_CAP_SPIRAM);
            gData->numHarpoons        = 250;
            gData->fuel = 1000 * 60 * 1; // 1 thousand milliseconds in a second. 60 seconds in a minute. 1 minutes.
            bb_setData(entity, gData);

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
            bb_rocketData_t* rData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_rocketData_t), MALLOC_CAP_SPIRAM);
            rData->flame           = NULL;
            rData->yVel            = 0;
            bb_setData(entity, rData);

            entity->collisions = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionRocket};
            push(entity->collisions, (void*)collision);

            entity->halfWidth  = 192;
            entity->halfHeight = 448;

            entity->updateFunction    = &bb_updateRocketLanding;
            break;
        }
        case HARPOON:
        {
            bb_projectileData_t* pData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_projectileData_t), MALLOC_CAP_SPIRAM);
            bb_setData(entity, pData);

            entity->collisions = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = HEAP_CAPS_CALLOC_DBG(1, sizeof(list_t), MALLOC_CAP_SPIRAM);

            // Neat trick  where you push the value of a bb_spriteDef_t as the pointer. Then when it pops, cast it
            // instead of deferencing and you're good to go! lists store a pointer, but you can abuse that and store any
            // 32 bits of info you want there, as long as you know how to handle it on the other end
            push(others, (void*)BU);
            push(others, (void*)BUG);
            push(others, (void*)BUGG);
            push(others, (void*)BUGGO);
            push(others, (void*)BUGGY);
            push(others, (void*)BUTT);

            bb_collision_t* collision = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionHarpoon};
            push(entity->collisions, (void*)collision);

            entity->updateFunction = &bb_updateHarpoon;
            entity->drawFunction   = &bb_drawHarpoon;
            break;
        }
        case EGG_LEAVES:
        {
            bb_eggLeavesData_t* elData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_eggLeavesData_t), MALLOC_CAP_SPIRAM);
            bb_setData(entity, elData);

            entity->updateFunction    = &bb_updateEggLeaves;
            entity->updateFarFunction = &bb_updateFarEggleaves;
            entity->drawFunction      = &bb_drawEggLeaves;
            break;
        }
        case EGG:
        {
            bb_eggData_t* eData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_eggData_t), MALLOC_CAP_SPIRAM);
            entity->data        = eData;

            entity->drawFunction = &bb_drawEgg;
            break;
        }
        case BU:
        {
            bb_bugData_t* bData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health       = 100;
            bb_setData(entity, bData);

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
            bb_bugData_t* bData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health       = 100;
            bb_setData(entity, bData);

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
            bb_bugData_t* bData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health       = 100;
            bb_setData(entity, bData);

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
            bb_bugData_t* bData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health       = 100;
            bb_setData(entity, bData);

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
            bb_bugData_t* bData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health       = 100;
            bb_setData(entity, bData);

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
            bb_bugData_t* bData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_bugData_t), MALLOC_CAP_SPIRAM);
            bData->health       = 100;
            bb_setData(entity, bData);

            entity->hasLighting                 = true;
            entity->gameFramesPerAnimationFrame = bb_randomInt(2, 4);

            entity->cacheable = true;

            entity->halfWidth  = 184;
            entity->halfHeight = 88;

            entity->updateFunction = &bb_updateBug;
            break;
        }
        case BB_MENU:
        {
            bb_menuData_t* mData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_menuData_t), MALLOC_CAP_SPIRAM);

            mData->cursor
                = bb_createEntity(entityManager, LOOPING_ANIMATION, false, HARPOON, 1,
                                  (entity->pos.x >> DECIMAL_BITS) - 22, 0, false);//y position doesn't matter here. It will be handled in the update loop.

            // This will make it draw pointed right
            ((bb_projectileData_t*)mData->cursor->data)->vel = (vec_t){10, 0};

            mData->cursor->updateFunction = NULL;

            bb_setData(entity, mData);

            entity->halfWidth  = 140;
            entity->halfHeight = 120;

            entity->updateFunction = &bb_updateMenu;
            entity->updateFarFunction = &bb_updateFarMenu;
            entity->drawFunction   = &bb_drawMenu;
            break;
        }
        case NO_SPRITE_STAR:
        {
            entity->drawFunction      = &bb_drawStar;
            entity->updateFarFunction = &bb_updateFarDestroy;
            break;
        }
        case NO_SPRITE_POI:
        {
            bb_setData(entity, HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_goToData), MALLOC_CAP_SPIRAM));
            //entity->updateFunction = &bb_updatePOI;
            entity->drawFunction   = &bb_drawNothing;
            break;
        }
        case OVO_TALK:
        {
            entityManager->sprites[OVO_TALK].originY = -240;
            entity->currentAnimationFrame = bb_randomInt(0,7);
            entity->updateFunction = &bb_updateCharacterTalk;
            entity->drawFunction = &bb_drawCharacterTalk;
            break;
        }
        case ATTACHMENT_ARM:
        {
            bb_attachmentArmData_t* aData = HEAP_CAPS_CALLOC_DBG(1, sizeof(bb_attachmentArmData_t), MALLOC_CAP_SPIRAM);
            aData->angle = 359;
            bb_setData(entity, aData);
            entity->updateFunction = &bb_updateAttachmentArm;
            entity->drawFunction = &bb_drawAttachmentArm;
            break;
        }
        default: // FLAME_ANIM and others need nothing set
        {
            break;
        }
    }
    entity->cSquared = entity->halfWidth * entity->halfWidth + entity->halfHeight * entity->halfHeight;

    entityManager->activeEntities++;
    if (entityManager->activeEntities > MAX_ENTITIES - 10 || entityManager->activeEntities < 10
        || entityManager->activeEntities % 25 == 0)
    {
        printf("%d/%d entities ^\n", entityManager->activeEntities, MAX_ENTITIES);
    }

    return entity;
}

void bb_freeEntityManager(bb_entityManager_t* self)
{
    for (uint8_t i = 0; i < NUM_SPRITES; i++)
    {
        bb_sprite_t* sprite = &self->sprites[i];
        for (uint8_t brightness = 0; brightness < sprite->brightnessLevels; brightness++)
        {
            for (uint8_t f = 0; f < sprite->numFrames; f++)
            {
                freeWsg(&sprite->frames[brightness * sprite->numFrames + f]);
            }
        }
        FREE_DBG(sprite->frames);
    }
    // free and clear
    FREE_DBG(self->entities);

    bb_entity_t* curEntity;
    while(NULL != (curEntity = pop(self->cachedEntities)))
    {
        bb_destroyEntity(curEntity, false);
        FREE_DBG(curEntity);
    }

    FREE_DBG(self->cachedEntities);
}