//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <esp_log.h>
#include <string.h>
#include <limits.h>

#include "mode_bigbug.h"
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

//==============================================================================
// Functions
//==============================================================================
void bb_initializeEntityManager(bb_entityManager_t* entityManager, bb_gameData_t* gameData)
{
    bb_loadSprites(entityManager);
    entityManager->entities = heap_caps_calloc_tag(MAX_ENTITIES, sizeof(bb_entity_t), MALLOC_CAP_SPIRAM, "entities");
    entityManager->frontEntities
        = heap_caps_calloc_tag(MAX_FRONT_ENTITIES, sizeof(bb_entity_t), MALLOC_CAP_SPIRAM, "frontEntities");

    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        bb_initializeEntity(&(entityManager->entities[i]), entityManager, gameData);
    }

    for (int i = 0; i < MAX_FRONT_ENTITIES; i++)
    {
        bb_initializeEntity(&(entityManager->frontEntities[i]), entityManager, gameData);
    }

    entityManager->activeEntities = 0;

    // Use calloc to ensure members are all 0 or NULL
    entityManager->cachedEntities = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "cachedEntities");
}

bb_sprite_t* bb_loadSprite(const char name[], uint8_t num_frames, uint8_t brightnessLevels, bb_sprite_t* sprite)
{
    sprite->brightnessLevels = brightnessLevels;
    if (!sprite->allocated)
    {
        sprite->numFrames = num_frames;
        sprite->frames    = heap_caps_calloc_tag(brightnessLevels * num_frames, sizeof(wsg_t), MALLOC_CAP_SPIRAM, name);
        sprite->allocated = true;
    }

    for (uint8_t brightness = 0; brightness < brightnessLevels; brightness++)
    {
        for (uint8_t i = 0; i < num_frames; i++)
        {
            wsg_t* wsg = &sprite->frames[brightness * num_frames + i];
            if (0 == wsg->h && 0 == wsg->w)
            {
                char wsg_name[strlen(name) + 8]; // 7 extra characters makes room for up to a 3 digit number + ".wsg" +
                                                 // null terminator ('\0')
                snprintf(wsg_name, sizeof(wsg_name), "%s%d.wsg", name, brightness * num_frames + i);
                loadWsgInplace(wsg_name, &sprite->frames[brightness * num_frames + i], true, bb_decodeSpace, bb_hsd);
            }
        }
    }

    return sprite;
}

void bb_freeSprite(bb_sprite_t* sprite)
{
    if (sprite->allocated)
    {
        for (uint8_t brightness = 0; brightness < sprite->brightnessLevels; brightness++)
        {
            for (uint8_t frame = 0; frame < sprite->numFrames; frame++)
            {
                if (sprite->frames[brightness * sprite->numFrames + frame].w
                    || sprite->frames[brightness * sprite->numFrames + frame].h)
                {
                    freeWsg(&sprite->frames[brightness * sprite->numFrames + frame]);
                }
            }
        }
        heap_caps_free(sprite->frames);
        sprite->allocated = false;
    }
}

void bb_loadSprites(bb_entityManager_t* entityManager)
{
    bb_loadSprite("crumble", 21, 1, &entityManager->sprites[CRUMBLE_ANIM]);
    entityManager->sprites[CRUMBLE_ANIM].originX = 48;
    entityManager->sprites[CRUMBLE_ANIM].originY = 22;

    bb_loadSprite("hit", 7, 1, &entityManager->sprites[BUMP_ANIM]);
    entityManager->sprites[BUMP_ANIM].originX = 37;
    entityManager->sprites[BUMP_ANIM].originY = 37;

    bb_loadSprite("rocket", 42, 1, &entityManager->sprites[ROCKET_ANIM]);
    entityManager->sprites[ROCKET_ANIM].originX = 33;
    entityManager->sprites[ROCKET_ANIM].originY = 66;
    // unload frames 1 through 39. Loaded when needed.
    for (int i = 1; i < 40; i++)
    {
        freeWsg(&entityManager->sprites[ROCKET_ANIM].frames[i]);
    }

    bb_loadSprite("flame", 11, 1, &entityManager->sprites[FLAME_ANIM]);
    entityManager->sprites[FLAME_ANIM].originX = 27;
    entityManager->sprites[FLAME_ANIM].originY = -28;

    bb_loadSprite("garbotnik-", 3, 1, &entityManager->sprites[GARBOTNIK_FLYING]);
    entityManager->sprites[GARBOTNIK_FLYING].originX = 18;
    entityManager->sprites[GARBOTNIK_FLYING].originY = 17;

    bb_loadSprite("harpoon-", 18, 1, &entityManager->sprites[HARPOON]);
    entityManager->sprites[HARPOON].originX = 10;
    entityManager->sprites[HARPOON].originY = 8;

    bb_loadSprite("eggLeaves", 1, 6, &entityManager->sprites[EGG_LEAVES]);
    entityManager->sprites[EGG_LEAVES].originX = 12;
    entityManager->sprites[EGG_LEAVES].originY = 5;

    bb_loadSprite("egg", 1, 6, &entityManager->sprites[EGG]);
    entityManager->sprites[EGG].originX = 12;
    entityManager->sprites[EGG].originY = 12;

    bb_loadSprite("bu", 4, 6, &entityManager->sprites[BU]);
    entityManager->sprites[BU].originX = 13;
    entityManager->sprites[BU].originY = 15;

    bb_loadSprite("bug", 4, 6, &entityManager->sprites[BUG]);
    entityManager->sprites[BUG].originX = 13;
    entityManager->sprites[BUG].originY = 6;

    bb_loadSprite("bugg", 4, 6, &entityManager->sprites[BUGG]);
    entityManager->sprites[BUGG].originX = 11;
    entityManager->sprites[BUGG].originY = 11;

    bb_loadSprite("buggo", 4, 6, &entityManager->sprites[BUGGO]);
    entityManager->sprites[BUGGO].originX = 12;
    entityManager->sprites[BUGGO].originY = 14;

    bb_loadSprite("buggy", 4, 6, &entityManager->sprites[BUGGY]);
    entityManager->sprites[BUGGY].originX = 13;
    entityManager->sprites[BUGGY].originY = 10;

    bb_loadSprite("butt", 4, 6, &entityManager->sprites[BUTT]);
    entityManager->sprites[BUTT].originX = 14;
    entityManager->sprites[BUTT].originY = 6;

    bb_loadSprite("bb_menu", 4, 1, &entityManager->sprites[BB_MENU]);
    entityManager->sprites[BB_MENU].originX = 140;
    entityManager->sprites[BB_MENU].originY = 354;

    bb_loadSprite("AttachmentArm", 1, 1, &entityManager->sprites[ATTACHMENT_ARM]);
    entityManager->sprites[ATTACHMENT_ARM].originX = 6;
    entityManager->sprites[ATTACHMENT_ARM].originY = 20;

    bb_loadSprite("WashingMachine", 1, 6, &entityManager->sprites[BB_WASHING_MACHINE]);
    entityManager->sprites[BB_WASHING_MACHINE].originX = 16;
    entityManager->sprites[BB_WASHING_MACHINE].originY = 16;

    // car is unloaded until needed.
    // bb_loadSprite("car", 60, 1, &entityManager->sprites[BB_CAR]);
    entityManager->sprites[BB_CAR].originX = 31;
    entityManager->sprites[BB_CAR].originY = 15;

    bb_loadSprite("skeleton", 1, 6, &entityManager->sprites[BB_SKELETON]);
    entityManager->sprites[BB_SKELETON].originX = 14;
    entityManager->sprites[BB_SKELETON].originY = 14;

    bb_loadSprite("fuel", 5, 1, &entityManager->sprites[BB_FUEL]);
    entityManager->sprites[BB_FUEL].originX = 7;
    entityManager->sprites[BB_FUEL].originY = 5;

    // grabby hand is unloaded until needed.
    // bb_loadSprite("grab", 3, 1, &entityManager->sprites[BB_GRABBY_HAND]);
    entityManager->sprites[BB_GRABBY_HAND].originX = 15;
    entityManager->sprites[BB_GRABBY_HAND].originY = -26;

    // door is unloaded until needed.
    // bb_loadSprite("door", 2, 1, &entityManager->sprites[BB_DOOR]);
    entityManager->sprites[BB_DOOR].originX = 16;
    entityManager->sprites[BB_DOOR].originY = 48;

    bb_loadSprite("donut", 1, 1, &entityManager->sprites[BB_DONUT]);
    entityManager->sprites[BB_DONUT].originX = 15;
    entityManager->sprites[BB_DONUT].originY = 15;

    // swadge is unloaded until needed.
    // bb_loadSprite("swadge", 12, 1, &entityManager->sprites[BB_SWADGE]);
    entityManager->sprites[BB_SWADGE].originX = 16;
    entityManager->sprites[BB_SWADGE].originY = 9;

    // food cart is unloaded until needed.
    // bb_loadSprite("foodCart", 2, 1, &entityManager->sprites[BB_FOOD_CART]);
    entityManager->sprites[BB_FOOD_CART].originX = 36;
    entityManager->sprites[BB_FOOD_CART].originY = 56;

    bb_loadSprite("wile", 1, 1, &entityManager->sprites[BB_WILE]);
    entityManager->sprites[BB_WILE].originX = 6;
    entityManager->sprites[BB_WILE].originY = 6;

    if (!entityManager->sprites[BB_ARROW].allocated)
    {
        entityManager->sprites[BB_ARROW].numFrames = 2;
        entityManager->sprites[BB_ARROW].frames
            = heap_caps_calloc_tag(2, sizeof(wsg_t), MALLOC_CAP_SPIRAM, "arrowFrames");
        loadWsgInplace("sh_up.wsg", &entityManager->sprites[BB_ARROW].frames[0], true, bb_decodeSpace, bb_hsd);
        loadWsgInplace("sh_u1.wsg", &entityManager->sprites[BB_ARROW].frames[1], true, bb_decodeSpace, bb_hsd);
        entityManager->sprites[BB_ARROW].allocated        = true;
        entityManager->sprites[BB_ARROW].brightnessLevels = 1;
    }

    if (!entityManager->sprites[BB_HOTDOG].allocated)
    {
        entityManager->sprites[BB_HOTDOG].numFrames = 1;
        entityManager->sprites[BB_HOTDOG].frames    = heap_caps_calloc(1, sizeof(wsg_t), MALLOC_CAP_SPIRAM);
        loadWsgInplace("hotdog_rs.wsg", &entityManager->sprites[BB_HOTDOG].frames[0], true, bb_decodeSpace, bb_hsd);
        entityManager->sprites[BB_HOTDOG].originX          = 6;
        entityManager->sprites[BB_HOTDOG].originY          = 6;
        entityManager->sprites[BB_HOTDOG].allocated        = true;
        entityManager->sprites[BB_HOTDOG].brightnessLevels = 1;
    }

    // final boss is unloaded until needed.
    entityManager->sprites[BB_FINAL_BOSS].originX = 30;
    entityManager->sprites[BB_FINAL_BOSS].originY = 40;
}

void bb_updateEntities(bb_entityManager_t* entityManager, bb_camera_t* camera)
{
    vec_t shiftedCameraPos = camera->camera.pos;
    shiftedCameraPos.x     = (shiftedCameraPos.x + 140) << DECIMAL_BITS;
    shiftedCameraPos.y     = (shiftedCameraPos.y + 120) << DECIMAL_BITS;
    node_t* currentNode    = entityManager->cachedEntities->first;
    bool isPaused          = entityManager->entities[0].gameData->isPaused;
    // This loop loads entities back in if they are close to the camera.
    while (currentNode != NULL && !isPaused)
    {
        bb_entity_t* curEntity = (bb_entity_t*)currentNode->val;
        node_t* next           = currentNode->next;

        // Do a rectangular bounds check that is somewhat larger than the camera itself. So stuff loads in and updates
        // slightly out of view.
        if (curEntity->pos.x > shiftedCameraPos.x - 3200 && curEntity->pos.x < shiftedCameraPos.x + 3200
            && curEntity->pos.y > shiftedCameraPos.y - 2880 && curEntity->pos.y < shiftedCameraPos.y + 2880)
        { // if it is close
            bb_entity_t* foundSpot = bb_findInactiveEntity(entityManager);
            if (foundSpot != NULL)
            {
                // load sprites if it is a car, grabbyHand, door, swadge, or foodCart
                switch (curEntity->spriteIndex)
                {
                    case BB_CAR:
                    {
                        bb_loadSprite("car", 60, 1, &entityManager->sprites[BB_CAR]);
                        // if it is on frame 59, unload frames 0 through 58
                        if (curEntity->currentAnimationFrame == 59)
                        {
                            for (int frame = 1; frame < 59; frame++)
                            {
                                freeWsg(&entityManager->sprites[BB_CAR].frames[frame]);
                            }
                        }
                        break;
                    }
                    case BB_DOOR:
                    {
                        bb_loadSprite("door", 2, 1, &entityManager->sprites[BB_DOOR]);
                        break;
                    }
                    case BB_SWADGE:
                    {
                        bb_loadSprite("swadge", 12, 1, &entityManager->sprites[BB_SWADGE]);
                        break;
                    }
                    case BB_FOOD_CART:
                    {
                        bb_foodCartData_t* fcData = (bb_foodCartData_t*)curEntity->data;
                        // tell this partner of the change in address
                        ((bb_foodCartData_t*)fcData->partner->data)->partner = foundSpot;
                        bb_loadSprite("foodCart", 2, 1, &entityManager->sprites[BB_FOOD_CART]);
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }

                // like a memcopy
                *foundSpot = *curEntity;
                entityManager->activeEntities++;
                heap_caps_free(removeEntry(entityManager->cachedEntities, currentNode));

                if (foundSpot->spriteIndex == EGG_LEAVES || foundSpot->spriteIndex == BB_SKELETON)
                {
                    // inform the tilemap of this uncached embedded entity address.
                    entityManager->activeBooster->gameData->tilemap
                        .fgTiles[foundSpot->pos.x >> 9][foundSpot->pos.y >> 9]
                        .entity
                        = foundSpot;
                }
            }
        }
        currentNode = next;
    }

    // This loops over all entities, doing updates and collision checks and moving the camera to the viewEntity.
    for (uint8_t i = 0; i < MAX_ENTITIES + MAX_FRONT_ENTITIES; i++)
    {
        bb_entity_t* curEntity;
        if (i < MAX_ENTITIES)
        {
            curEntity = &entityManager->entities[i];
        }
        else
        {
            curEntity = &entityManager->frontEntities[i - MAX_ENTITIES];
        }

        if (curEntity->active)
        {
            if (curEntity->cacheable)
            {
                if (!(curEntity->pos.x > shiftedCameraPos.x - 3200 && curEntity->pos.x < shiftedCameraPos.x + 3200
                      && curEntity->pos.y > shiftedCameraPos.y - 2880 && curEntity->pos.y < shiftedCameraPos.y + 2880))
                { // if it is far
                    // This entity gets cached
                    bb_entity_t* cachedEntity = heap_caps_calloc(1, sizeof(bb_entity_t), MALLOC_CAP_SPIRAM);
                    // It's like a memcopy
                    *cachedEntity = *curEntity;

                    switch (cachedEntity->spriteIndex)
                    {
                        case BB_FOOD_CART:
                        {
                            // tell this partner of the change in address
                            ((bb_foodCartData_t*)((bb_foodCartData_t*)cachedEntity->data)->partner->data)->partner
                                = cachedEntity;
                            break;
                        }
                        case BB_SKELETON:
                        {
                            // tell the tilemap of the change in address
                            cachedEntity->gameData->tilemap.fgTiles[cachedEntity->pos.x >> 9][cachedEntity->pos.y >> 9]
                                .entity
                                = cachedEntity;
                            break;
                        }
                        case EGG_LEAVES:
                        {
                            // tell the tilemap of the change in address
                            cachedEntity->gameData->tilemap.fgTiles[cachedEntity->pos.x >> 9][cachedEntity->pos.y >> 9]
                                .entity
                                = cachedEntity;
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }

                    // push to the tail
                    push(entityManager->cachedEntities, (void*)cachedEntity);
                    bb_destroyEntity(curEntity, true, true);
                    continue;
                }
            }

            if (curEntity->updateFunction != NULL
                && (!isPaused || curEntity->spriteIndex == NO_SPRITE_POI || curEntity->spriteIndex == OVO_TALK))
            {
                curEntity->updateFunction(curEntity);
            }
            if (curEntity->updateFarFunction != NULL
                && (!isPaused || curEntity->spriteIndex == NO_SPRITE_POI || curEntity->spriteIndex == OVO_TALK))
            {
                // 2752 = (140+32) << 4; 2432 = (120+32) << 4
                if (bb_boxesCollide(&(bb_entity_t){.pos = shiftedCameraPos, .halfWidth = 2752, .halfHeight = 2432},
                                    curEntity, NULL, NULL)
                    == false)
                {
                    curEntity->updateFarFunction(curEntity);
                }
                else if (curEntity->spriteIndex == BB_GRABBY_HAND && !entityManager->sprites[BB_GRABBY_HAND].allocated)
                {
                    bb_loadSprite("grab", 3, 1, &entityManager->sprites[BB_GRABBY_HAND]);
                    curEntity->drawFunction = bb_drawGrabbyHand;
                }
            }

            if (curEntity->collisions != NULL)
            {
                node_t* currentCollisionCheck = curEntity->collisions->first;
                while (currentCollisionCheck != NULL)
                {
                    bb_collision_t* collisionInfo = (bb_collision_t*)currentCollisionCheck->val;
                    if (entityManager->playerEntity != NULL && GARBOTNIK_DATA == entityManager->playerEntity->dataType
                        && ((bb_spriteDef_t)collisionInfo->checkOthers->first->val) == GARBOTNIK_FLYING
                        && collisionInfo->checkOthers->first->next == NULL)
                    {
                        // no need to search all other entities if it's simply something to do with the player.
                        // do a collision check here
                        bb_hitInfo_t hitInfo = {0};
                        if (bb_boxesCollide(curEntity, entityManager->playerEntity,
                                            &(((bb_garbotnikData_t*)entityManager->playerEntity->data)->previousPos),
                                            &hitInfo))
                        {
                            ((bb_collision_t*)currentCollisionCheck->val)
                                ->function(curEntity, entityManager->playerEntity, &hitInfo);
                        }
                        if (curEntity->collisions != NULL)
                        {
                            currentCollisionCheck = currentCollisionCheck->next;
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        for (uint8_t j = 0; j < MAX_ENTITIES; j++)
                        {
                            bb_entity_t* collisionCandidate = &entityManager->entities[j];
                            // Iterate over all nodes
                            currentCollisionCheck = curEntity->collisions->first;
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
                                        bb_hitInfo_t hitInfo = {0};
                                        if (bb_boxesCollide(curEntity, collisionCandidate, &collisionCandidate->pos,
                                                            &hitInfo))
                                        {
                                            ((bb_collision_t*)currentCollisionCheck->val)
                                                ->function(curEntity, collisionCandidate, &hitInfo);
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
    if (camera->camera.pos.y < -800 && camera->camera.pos.y > -2800)
    {
        int16_t halfWidth  = HALF_WIDTH >> DECIMAL_BITS;
        int16_t halfHeight = HALF_HEIGHT >> DECIMAL_BITS;

        int skewedChance = 0;
        if (camera->camera.pos.y > -2500)
        {
            skewedChance = 128 * camera->camera.pos.y + 320000;
        }

        if (bb_randomInt(1, 13000 + skewedChance) < (abs(camera->velocity.x) * camera->camera.height))
        {
            bb_createEntity(entityManager, NO_ANIMATION, true, NO_SPRITE_STAR, 1,
                            camera->velocity.x > 0 ? camera->camera.pos.x + 2 * halfWidth : camera->camera.pos.x,
                            camera->camera.pos.y + halfHeight + bb_randomInt(-halfHeight, halfHeight), false, false);
        }
        if (bb_randomInt(1, 13000 + skewedChance) < (abs(camera->velocity.y) * camera->camera.width))
        {
            bb_createEntity(entityManager, NO_ANIMATION, true, NO_SPRITE_STAR, 1,
                            camera->camera.pos.x + halfWidth + bb_randomInt(-halfWidth, halfWidth),
                            camera->velocity.y > 0 ? camera->camera.pos.y + 2 * halfHeight : camera->camera.pos.y,
                            false, false);
        }
        camera->velocity.x = 0;
        camera->velocity.y = 0;
    }
}

void bb_deactivateAllEntities(bb_entityManager_t* entityManager, bool excludePersistentEntities)
{
    for (uint8_t i = 0; i < MAX_ENTITIES; i++)
    {
        bb_entity_t* currentEntity = &(entityManager->entities[i]);
        if (!currentEntity->active)
        {
            continue;
        }
        if (excludePersistentEntities
            && (currentEntity->spriteIndex == FLAME_ANIM || currentEntity->spriteIndex == NO_SPRITE_STAR))
        {
            continue;
        }
        bb_destroyEntity(currentEntity, false, true);
    }

    if (!excludePersistentEntities)
    {
        for (uint8_t i = 0; i < MAX_FRONT_ENTITIES; i++)
        {
            bb_entity_t* currentEntity = &(entityManager->frontEntities[i]);
            if (!currentEntity->active)
            {
                continue;
            }
            bb_destroyEntity(currentEntity, false, false);
        }
    }

    // destroy all cached entities
    bb_entity_t* curEntity;
    while (NULL != (curEntity = pop(entityManager->cachedEntities)))
    {
        bb_destroyEntity(curEntity, false, false);
        heap_caps_free(curEntity);
    }
}

void bb_drawEntity(bb_entity_t* currentEntity, bb_entityManager_t* entityManager, rectangle_t* camera)
{
    if (currentEntity->drawFunction != NULL)
    {
        currentEntity->drawFunction(entityManager, camera, currentEntity);
    }
    // If the sprite is out-of-bounds
    else if (currentEntity->spriteIndex >= NUM_SPRITES
             // Or the sprite is in-bounds, but not allocated
             || !entityManager->sprites[currentEntity->spriteIndex].allocated)
    {
        // Immediately return
        return;
    }
    else if (entityManager->sprites[currentEntity->spriteIndex].brightnessLevels == 6)
    {
        uint8_t brightness = 5;
        int16_t xOff       = (currentEntity->pos.x >> DECIMAL_BITS)
                       - entityManager->sprites[currentEntity->spriteIndex].originX - camera->pos.x;
        int16_t yOff = (currentEntity->pos.y >> DECIMAL_BITS)
                       - entityManager->sprites[currentEntity->spriteIndex].originY - camera->pos.y;
        if (entityManager->playerEntity != NULL)
        {
            vec_t lookup
                = {.x = (currentEntity->pos.x >> DECIMAL_BITS) - (entityManager->playerEntity->pos.x >> DECIMAL_BITS)
                        + currentEntity->gameData->tilemap.headlampWsg.w,
                   .y = (currentEntity->pos.y >> DECIMAL_BITS) - (entityManager->playerEntity->pos.y >> DECIMAL_BITS)
                        + currentEntity->gameData->tilemap.headlampWsg.h};

            lookup = divVec2d(lookup, 2);
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

            if (GARBOTNIK_DATA == currentEntity->gameData->entityManager.playerEntity->dataType)
            {
                brightness = bb_midgroundLighting(
                    &(currentEntity->gameData->tilemap.headlampWsg), &lookup,
                    &(((bb_garbotnikData_t*)currentEntity->gameData->entityManager.playerEntity->data)->yaw.x),
                    brightness);
            }
        }
        drawWsgSimple(&entityManager->sprites[currentEntity->spriteIndex]
                           .frames[brightness + currentEntity->currentAnimationFrame * 6],
                      xOff, yOff);
    }
    else
    {
        drawWsgSimple(&entityManager->sprites[currentEntity->spriteIndex].frames[currentEntity->currentAnimationFrame],
                      (currentEntity->pos.x >> DECIMAL_BITS)
                          - entityManager->sprites[currentEntity->spriteIndex].originX - camera->pos.x,
                      (currentEntity->pos.y >> DECIMAL_BITS)
                          - entityManager->sprites[currentEntity->spriteIndex].originY - camera->pos.y);
    }

    if (!currentEntity->paused && !currentEntity->gameData->isPaused)
    {
        // increment the frame counter
        currentEntity->animationTimer++;
        currentEntity->currentAnimationFrame
            = currentEntity->animationTimer / currentEntity->gameFramesPerAnimationFrame;
        // if frame reached the end of the animation
        if (currentEntity->spriteIndex < NUM_SPRITES
            && currentEntity->currentAnimationFrame >= entityManager->sprites[currentEntity->spriteIndex].numFrames)
        {
            switch (currentEntity->type)
            {
                case ONESHOT_ANIMATION:
                {
                    // destroy the entity
                    bb_destroyEntity(currentEntity, false, true);
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

void bb_drawEntities(bb_entityManager_t* entityManager, rectangle_t* camera)
{
    for (int i = 0; i < MAX_ENTITIES + MAX_FRONT_ENTITIES; i++)
    {
        bb_entity_t* currentEntity;
        if (i < MAX_ENTITIES)
        {
            currentEntity = &entityManager->entities[i];
        }
        else
        {
            currentEntity = &entityManager->frontEntities[i - MAX_ENTITIES];
        }
        if (currentEntity->active)
        {
            bb_drawEntity(currentEntity, entityManager, camera);
        }
    }
}

bb_entity_t* bb_findInactiveFrontEntity(bb_entityManager_t* entityManager)
{
    for (int i = 0; i < MAX_FRONT_ENTITIES; i++)
    {
        if (entityManager->frontEntities[i].active == false)
        {
            return &entityManager->frontEntities[i];
        }
    }
    return NULL;
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
            return &entityManager->entities[i];
        }
    }
    return NULL;
}

bb_entity_t* bb_findInactiveFrontEntityBackwards(bb_entityManager_t* entityManager)
{
    for (int i = MAX_FRONT_ENTITIES - 1; i >= 0; i--)
    {
        if (entityManager->frontEntities[i].active == false)
        {
            return &entityManager->frontEntities[i];
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
            return &entityManager->entities[i];
        }
    }
    return NULL;
}

// This function destroys enough unimportant entities to make from for more.
void bb_ensureEntitySpace(bb_entityManager_t* entityManager, uint8_t numEntities)
{
    if (entityManager->activeEntities <= MAX_ENTITIES - numEntities)
    {
        return;
    }
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        if (entityManager->entities[i].active
            && (entityManager->entities[i].spriteIndex == CRUMBLE_ANIM
                || entityManager->entities[i].spriteIndex == BUMP_ANIM
                || entityManager->entities[i].spriteIndex == BB_SPIT))
        {
            bb_destroyEntity(&entityManager->entities[i], false, true);
            if (entityManager->activeEntities <= MAX_ENTITIES - numEntities)
            {
                return;
            }
        }
    }
    for (int i = 0; i < MAX_ENTITIES; i++)
    {
        if (entityManager->entities[i].active &&

            (entityManager->entities[i].spriteIndex >= 8
             && entityManager->entities[i].spriteIndex <= 13) // bugs are 8 through 13
        )
        {
            bb_destroyEntity(&entityManager->entities[i], false, true);
            if (entityManager->activeEntities <= MAX_ENTITIES - numEntities)
            {
                return;
            }
        }
    }
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
    camera->velocity
        = addVec2d(camera->velocity,
                   subVec2d(camera->camera.pos, previousPos)); // velocity is this position minus previous position.
}

bb_entity_t* bb_createEntity(bb_entityManager_t* entityManager, bb_animationType_t type, bool paused,
                             bb_spriteDef_t spriteIndex, uint8_t gameFramesPerAnimationFrame, uint32_t x, uint32_t y,
                             bool renderFront, bool forceToFront)
{
    if (!forceToFront && entityManager->activeEntities == MAX_ENTITIES)
    {
        // ESP_LOGD(BB_TAG,"Failed entity creation. MAX_ENTITIES exceeded.\n");
        return NULL;
    }

    bb_entity_t* entity;
    if (forceToFront)
    {
        if (renderFront)
        {
            entity = bb_findInactiveFrontEntityBackwards(entityManager);
        }
        else
        {
            entity = bb_findInactiveFrontEntity(entityManager);
        }
    }
    else
    {
        if (renderFront)
        {
            entity = bb_findInactiveEntityBackwards(entityManager);
        }
        else
        {
            entity = bb_findInactiveEntity(entityManager);
        }
    }

    if (entity == NULL)
    {
        ESP_LOGD(BB_TAG, "entityManager_bigbug.c YOOOOO! This should never happen.\n");
        return NULL;
    }

    entity->active = true;
    entity->pos.x  = x << DECIMAL_BITS;
    entity->pos.y  = y << DECIMAL_BITS;

    entity->type   = type;
    entity->paused = paused;
    if (!(entity->gameData->tutorialFlags & 0b1) && spriteIndex > 7 && spriteIndex < 14
        && entity->gameData->entityManager.playerEntity != NULL)
    {
        if (entity->pos.y < 512 && (spriteIndex == BUGG || spriteIndex == BUGGO))
        {
            // don't let the tutorial bug be a flying type.
            spriteIndex += 2;
        }
        entity->gameData->isPaused = true;
        // set the tutorial flag
        entity->gameData->tutorialFlags |= 0b1;
        bb_entity_t* ovo = bb_createEntity(&entity->gameData->entityManager, NO_ANIMATION, true, OVO_TALK, 1,
                                           entity->gameData->camera.camera.pos.x, entity->gameData->camera.camera.pos.y,
                                           false, true);
        if (ovo != NULL)
        {
            bb_dialogueData_t* dData = bb_createDialogueData(11, "Ovo");

            bb_setCharacterLine(dData, 0, "Ovo", "Wow, look at that juicy bug!");
            bb_setCharacterLine(dData, 1, "Ovo", "Ah, who am I kidding?");
            bb_setCharacterLine(dData, 2, "Ovo", "My time in this landfill is limited.");
            bb_setCharacterLine(dData, 3, "Ovo",
                                "So I'm going to break the fourth wall and tell you how to play the freaking game!");
            bb_setCharacterLine(
                dData, 4, "Ovo",
                "There are a bunch of ways to incapacitate that bug. But let's learn how to use harpoons first.");
            bb_setCharacterLine(dData, 5, "Ovo", "Hold your right thumb on the  C-Touchpad to aim.");
            bb_setCharacterLine(dData, 6, "Ovo", "Hey! Wait until I'm done talking, you doofus.");
            bb_setCharacterLine(dData, 7, "Ovo",
                                "If your touch vector is outside of the purple circle that appears on-screen, harpoons "
                                "will fire steadily.");
            bb_setCharacterLine(dData, 8, "Ovo", "Three hits will flip the bug upside down!");
            bb_setCharacterLine(
                dData, 9, "Ovo",
                "There's a nifty detail where harpoons with upward velocity pass right through terrain.");
            bb_setCharacterLine(dData, 10, "Ovo", "Now, let's get that bug!");

            dData->curString     = -1;
            dData->endDialogueCB = &bb_afterGarbotnikTutorialTalk;
            bb_setData(ovo, dData, DIALOGUE_DATA);
        }
    }
    entity->spriteIndex = spriteIndex;

    entity->gameFramesPerAnimationFrame = gameFramesPerAnimationFrame;

    switch (spriteIndex)
    {
        case GARBOTNIK_FLYING:
        {
            bb_garbotnikData_t* gData = heap_caps_calloc(1, sizeof(bb_garbotnikData_t), MALLOC_CAP_SPIRAM);
            gData->numHarpoons        = entity->gameData->GarbotnikStat_maxHarpoons;
            gData->fuel = 1000 * 60 * 3; // 1 thousand milliseconds in a second. 60 seconds in a minute. 3 minutes.
                                         // //also set in bb_onCollisionFuel()
            gData->activeWile = 255;     // 255 means no wile active.
            gData->dragShift  = 17;

            gData->activationRadius = 1102;

            memset(&gData->towedEntities, 0, sizeof(list_t));
            int16_t arraySize = sizeof(gData->landingPhrases) / sizeof(gData->landingPhrases[0]);
            // create sequential numbers of all phrase indices
            for (int16_t i = 0; i < arraySize; i++)
                gData->landingPhrases[i] = i;
            // shuffle the array
            for (int16_t i = arraySize - 1; i > 0; i--)
            {
                int16_t j                = (int16_t)(bb_randomInt(0, INT_MAX) % (i + 1));
                int16_t temp             = gData->landingPhrases[i];
                gData->landingPhrases[i] = gData->landingPhrases[j];
                gData->landingPhrases[j] = temp;
            }
            ESP_LOGD(BB_TAG, "shuffled:\n");
            for (int i = 0; i < arraySize; i++)
            {
                ESP_LOGD(BB_TAG, "%d\n", gData->landingPhrases[i]);
            }

            bb_setData(entity, gData, GARBOTNIK_DATA);

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
            bb_rocketData_t* rData = heap_caps_calloc_tag(1, sizeof(bb_rocketData_t), MALLOC_CAP_SPIRAM, "rData");
            rData->flame           = NULL;
            rData->yVel            = 0;
            rData->armAngle        = 2880; // That is 180 << DECIMAL_BITS
            bb_setData(entity, rData, ROCKET_DATA);

            entity->collisions = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rCollisions");
            list_t* others     = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rOthers");
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision
                = heap_caps_calloc_tag(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM, "rCollision");
            *collision = (bb_collision_t){others, bb_onCollisionRocketGarbotnik};
            push(entity->collisions, (void*)collision);

            list_t* others2 = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rOthers");
            // Neat trick  where you push the value of a bb_spriteDef_t as the pointer. Then when it pops, cast it
            // instead of deferencing and you're good to go! lists store a pointer, but you can abuse that and store any
            // 32 bits of info you want there, as long as you know how to handle it on the other end
            push(others2, (void*)BU);
            push(others2, (void*)BUG);
            push(others2, (void*)BUGG);
            push(others2, (void*)BUGGO);
            push(others2, (void*)BUGGY);
            push(others2, (void*)BUTT);
            push(others2, (void*)EGG);
            bb_collision_t* collision2
                = heap_caps_calloc_tag(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM, "rCollision");
            *collision2 = (bb_collision_t){others2, bb_onCollisionHeavyFallingBug};
            push(entity->collisions, (void*)collision2);

            entity->halfWidth    = 192;
            entity->halfHeight   = 448;
            entity->drawFunction = &bb_drawRocket;
            break;
        }
        case HARPOON:
        {
            bb_projectileData_t* pData = heap_caps_calloc(1, sizeof(bb_projectileData_t), MALLOC_CAP_SPIRAM);
            bb_setData(entity, pData, PROJECTILE_DATA);

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);

            // Neat trick  where you push the value of a bb_spriteDef_t as the pointer. Then when it pops, cast it
            // instead of deferencing and you're good to go! lists store a pointer, but you can abuse that and store any
            // 32 bits of info you want there, as long as you know how to handle it on the other end
            push(others, (void*)BU);
            push(others, (void*)BUG);
            push(others, (void*)BUGG);
            push(others, (void*)BUGGO);
            push(others, (void*)BUGGY);
            push(others, (void*)BUTT);
            push(others, (void*)EGG);

            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionHarpoon};
            push(entity->collisions, (void*)collision);

            entity->updateFunction = &bb_updateHarpoon;
            entity->drawFunction   = &bb_drawHarpoon;
            break;
        }
        case EGG_LEAVES:
        {
            bb_eggLeavesData_t* elData = heap_caps_calloc(1, sizeof(bb_eggLeavesData_t), MALLOC_CAP_SPIRAM);
            bb_setData(entity, elData, EGG_LEAVES_DATA);

            entity->updateFunction    = &bb_updateEggLeaves;
            entity->updateFarFunction = &bb_updateFarEggleaves;
            entity->drawFunction      = &bb_drawEggLeaves;
            break;
        }
        case EGG:
        {
            bb_setData(entity, heap_caps_calloc(1, sizeof(bb_eggData_t), MALLOC_CAP_SPIRAM), EGG_DATA);

            entity->halfWidth  = 140;
            entity->halfHeight = 165;

            entity->drawFunction = &bb_drawEgg;
            break;
        }
        case BU:
        {
            bb_walkingBugData_t* bData = heap_caps_calloc(1, sizeof(bb_walkingBugData_t), MALLOC_CAP_SPIRAM);
            bData->health              = 100;
            bData->gravity             = BB_DOWN;
            bData->speed               = 4 * bb_randomInt(3, 6);
            bData->flags               = bb_randomInt(0, 1);
            bb_setData(entity, bData, WALKING_BUG_DATA);

            entity->gameFramesPerAnimationFrame = (40 - bData->speed) / 5;

            entity->cacheable = true;

            entity->halfWidth  = 192;
            entity->halfHeight = 104;

            entity->updateFunction = &bb_updateWalkingBug;
            entity->drawFunction   = &bb_drawBug;
            break;
        }
        case BUG:
        {
            bb_walkingBugData_t* bData = heap_caps_calloc(1, sizeof(bb_walkingBugData_t), MALLOC_CAP_SPIRAM);
            bData->health              = 100;
            bData->gravity             = BB_DOWN;
            bData->speed               = 4 * bb_randomInt(1, 5);
            bData->flags               = bb_randomInt(0, 1);
            bb_setData(entity, bData, WALKING_BUG_DATA);

            entity->gameFramesPerAnimationFrame = (40 - bData->speed) / 5;

            entity->cacheable = true;

            entity->halfWidth  = 176;
            entity->halfHeight = 64;

            entity->updateFunction = &bb_updateWalkingBug;
            entity->drawFunction   = &bb_drawBug;
            break;
        }
        case BUGG:
        {
            bb_flyingBugData_t* bData = heap_caps_calloc(1, sizeof(bb_flyingBugData_t), MALLOC_CAP_SPIRAM);
            bData->health             = 100;
            bData->speed              = 4 * bb_randomInt(1, 5);
            bData->direction = rotateVec2d(divVec2d((vec_t){0, bData->speed * 200}, 800), bb_randomInt(0, 359));
            bData->flags     = bData->direction.x < 0;
            bb_setData(entity, bData, FLYING_BUG_DATA);

            entity->gameFramesPerAnimationFrame = (40 - bData->speed) / 5;

            entity->cacheable = true;

            entity->halfWidth  = 120;
            entity->halfHeight = 104;

            entity->updateFunction = &bb_updateFlyingBug;
            entity->drawFunction   = &bb_drawBug;
            break;
        }
        case BUGGO:
        {
            bb_flyingBugData_t* bData = heap_caps_calloc(1, sizeof(bb_flyingBugData_t), MALLOC_CAP_SPIRAM);
            bData->health             = 100;
            bData->speed              = 4 * bb_randomInt(3, 4);
            bData->direction = rotateVec2d(divVec2d((vec_t){0, bData->speed * 200}, 800), bb_randomInt(0, 359));
            bData->flags     = bData->direction.x < 0;
            bb_setData(entity, bData, FLYING_BUG_DATA);

            entity->gameFramesPerAnimationFrame = (40 - bData->speed) / 5;

            entity->cacheable = true;

            entity->halfWidth  = 144;
            entity->halfHeight = 144;

            entity->updateFunction = &bb_updateFlyingBug;
            entity->drawFunction   = &bb_drawBug;
            break;
        }
        case BUGGY:
        {
            bb_walkingBugData_t* bData = heap_caps_calloc(1, sizeof(bb_walkingBugData_t), MALLOC_CAP_SPIRAM);
            bData->health              = 100;
            bData->gravity             = BB_DOWN;
            bData->speed               = 4 * bb_randomInt(1, 5);
            bData->flags               = bb_randomInt(0, 1);
            bb_setData(entity, bData, WALKING_BUG_DATA);

            entity->gameFramesPerAnimationFrame = (40 - bData->speed) / 5;

            entity->cacheable = true;

            entity->halfWidth  = 184;
            entity->halfHeight = 80;

            entity->updateFunction = &bb_updateWalkingBug;
            entity->drawFunction   = &bb_drawBug;
            break;
        }
        case BUTT:
        {
            bb_walkingBugData_t* bData = heap_caps_calloc(1, sizeof(bb_walkingBugData_t), MALLOC_CAP_SPIRAM);
            bData->health              = 100;
            bData->gravity             = BB_DOWN;
            bData->speed               = 4 * bb_randomInt(1, 5);
            bData->flags               = bb_randomInt(0, 1);
            bb_setData(entity, bData, WALKING_BUG_DATA);

            entity->gameFramesPerAnimationFrame = (40 - bData->speed) / 5;

            entity->cacheable = true;

            entity->halfWidth  = 184;
            entity->halfHeight = 88;

            entity->updateFunction = &bb_updateWalkingBug;
            entity->drawFunction   = &bb_drawBug;
            break;
        }
        case BB_MENU:
        {
            bb_menuData_t* mData = heap_caps_calloc(1, sizeof(bb_menuData_t), MALLOC_CAP_SPIRAM);

            bb_ensureEntitySpace(entityManager, 1);
            mData->cursor = bb_createEntity(
                entityManager, LOOPING_ANIMATION, false, HARPOON, 3, (entity->pos.x >> DECIMAL_BITS) - 22, 0, false,
                false); // y position doesn't matter here. It will be handled in the update loop.

            if (mData->cursor != NULL)
            {
                bb_clearCollisions(mData->cursor, false);

                // This will make it draw pointed right
                ((bb_projectileData_t*)mData->cursor->data)->vel = (vec_t){10, 0};

                mData->cursor->updateFunction = NULL;
            }

            bb_setData(entity, mData, MENU_DATA);

            entity->halfWidth  = 140;
            entity->halfHeight = 120;

            entity->updateFunction    = &bb_updateMenu;
            entity->updateFarFunction = &bb_updateFarMenu;
            entity->drawFunction      = &bb_drawMenu;
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
            bb_setData(entity, heap_caps_calloc(1, sizeof(bb_goToData), MALLOC_CAP_SPIRAM), GO_TO_DATA);
            // entity->updateFunction = &bb_updatePOI;
            entity->drawFunction = &bb_drawNothing;
            break;
        }
        case OVO_TALK:
        {
            entity->updateFunction = &bb_updateCharacterTalk;
            entity->drawFunction   = &bb_drawCharacterTalk;
            break;
        }
        case ATTACHMENT_ARM:
        {
            bb_setData(entity, heap_caps_calloc(1, sizeof(bb_attachmentArmData_t), MALLOC_CAP_SPIRAM),
                       ATTACHMENT_ARM_DATA);
            entity->updateFunction = &bb_updateAttachmentArm;
            entity->drawFunction   = &bb_drawAttachmentArm;

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionAttachmentArm};
            push(entity->collisions, (void*)collision);
            break;
        }
        case BB_GAME_OVER:
        {
            bb_gameOverData_t* goData = heap_caps_calloc(1, sizeof(bb_gameOverData_t), MALLOC_CAP_SPIRAM);
            loadWsgInplace("GameOver0.wsg", &goData->fullscreenGraphic, true, bb_decodeSpace, bb_hsd);
            goData->wsgLoaded = true;
            bb_setData(entity, goData, GAME_OVER_DATA);

            entity->currentAnimationFrame = 0;
            entity->updateFunction        = &bb_updateGameOver;
            entity->drawFunction          = &bb_drawGameOver;
            break;
        }
        case BB_WASHING_MACHINE:
        {
            bb_setData(entity, heap_caps_calloc(1, sizeof(bb_heavyFallingData_t), MALLOC_CAP_SPIRAM),
                       HEAVY_FALLING_DATA);
            entity->halfWidth      = 16 << DECIMAL_BITS;
            entity->halfHeight     = 16 << DECIMAL_BITS;
            entity->updateFunction = &bb_updateHeavyFalling;
            entity->cacheable      = true;

            entity->collisions = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rCollisions");
            list_t* others     = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rOthers");
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision
                = heap_caps_calloc_tag(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM, "rCollision");
            *collision = (bb_collision_t){others, bb_onCollisionHeavyFallingGarbotnik};
            push(entity->collisions, (void*)collision);

            list_t* others2 = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rOthers");
            // Neat trick  where you push the value of a bb_spriteDef_t as the pointer. Then when it pops, cast it
            // instead of deferencing and you're good to go! lists store a pointer, but you can abuse that and store any
            // 32 bits of info you want there, as long as you know how to handle it on the other end
            push(others2, (void*)BU);
            push(others2, (void*)BUG);
            push(others2, (void*)BUGG);
            push(others2, (void*)BUGGO);
            push(others2, (void*)BUGGY);
            push(others2, (void*)BUTT);
            push(others2, (void*)EGG);
            bb_collision_t* collision2
                = heap_caps_calloc_tag(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM, "rCollision");
            *collision2 = (bb_collision_t){others2, bb_onCollisionHeavyFallingBug};
            push(entity->collisions, (void*)collision2);

            break;
        }
        case BB_CAR:
        {
            bb_setData(entity, heap_caps_calloc(1, sizeof(bb_carData_t), MALLOC_CAP_SPIRAM), CAR_DATA);
            entity->halfWidth  = 25 << DECIMAL_BITS;
            entity->halfHeight = 13 << DECIMAL_BITS;
            entity->cacheable  = true;

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionCarIdle};
            push(entity->collisions, (void*)collision);

            entity->drawFunction      = &bb_drawCar;
            entity->updateFarFunction = &bb_updateFarCar;

            // Load sprites just in time.
            bb_loadSprite("car", 60, 1, &entityManager->sprites[BB_CAR]);

            break;
        }
        case BB_DEATH_DUMPSTER:
        {
            bb_DeathDumpsterData_t* ddData = heap_caps_calloc(1, sizeof(bb_DeathDumpsterData_t), MALLOC_CAP_SPIRAM);

            ddData->loaded = false; // the wsg is not loaded
            bb_setData(entity, ddData, DEATH_DUMPSTER_DATA);

            entity->drawFunction = &bb_drawDeathDumpster;
            break;
        }
        case BB_SKELETON:
        {
            entity->drawFunction = &bb_drawBasicEmbed;
            break;
        }
        case BB_FUEL:
        {
            entity->cacheable = true;

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionFuel};
            push(entity->collisions, (void*)collision);
            break;
        }
        case BB_GRABBY_HAND:
        {
            bb_grabbyHandData_t* ghData
                = (bb_grabbyHandData_t*)heap_caps_calloc(1, sizeof(bb_grabbyHandData_t), MALLOC_CAP_SPIRAM);
            bb_setData(entity, ghData, GRABBY_HAND_DATA);

            entity->cacheable  = false;
            entity->halfWidth  = 7 << DECIMAL_BITS;
            entity->halfHeight = 26 << DECIMAL_BITS;

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            push(others, (void*)BU);
            push(others, (void*)BUG);
            push(others, (void*)BUGG);
            push(others, (void*)BUGGO);
            push(others, (void*)BUGGY);
            push(others, (void*)BUTT);
            push(others, (void*)BB_DONUT);
            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionGrabbyHand};
            push(entity->collisions, (void*)collision);

            entity->updateFunction    = &bb_updateGrabbyHand;
            entity->updateFarFunction = &bb_updateFarGrabbyHand;
            entity->drawFunction      = &bb_drawGrabbyHand;

            // sprites loaded just-in-time
            bb_loadSprite("grab", 3, 1, &entityManager->sprites[BB_GRABBY_HAND]);
            break;
        }
        case BB_RADAR_PING:
        {
            bb_radarPingData_t* rpData
                = (bb_radarPingData_t*)heap_caps_calloc(1, sizeof(bb_radarPingData_t), MALLOC_CAP_SPIRAM);
            rpData->timer = 500;
            bb_setData(entity, rpData, RADAR_PING_DATA);

            entity->updateFunction = &bb_updateRadarPing;
            entity->drawFunction   = &bb_drawRadarPing;
            break;
        }
        case BB_DOOR:
        {
            entity->halfWidth      = 12 << DECIMAL_BITS;
            entity->halfHeight     = 48 << DECIMAL_BITS;
            entity->cacheable      = true;
            entity->updateFunction = &bb_updateDoor;
            // sprites loaded just-in-time
            bb_loadSprite("door", 2, 1, &entityManager->sprites[BB_DOOR]);
            break;
        }
        case BB_JANKY_BUG_DIG:
        {
            entity->halfHeight          = 3;
            entity->halfWidth           = 3;
            bb_jankyBugDigData_t* jData = heap_caps_calloc(1, sizeof(bb_jankyBugDigData_t), MALLOC_CAP_SPIRAM);
            jData->numberOfDigs         = 0;
            bb_setData(entity, jData, JANKY_BUG_DIG_DATA);

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);

            push(others, (void*)BU);
            push(others, (void*)BUG);
            push(others, (void*)BUGG);
            push(others, (void*)BUGGO);
            push(others, (void*)BUGGY);
            push(others, (void*)BUTT);

            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionJankyBugDig};
            push(entity->collisions, (void*)collision);

            entity->drawFunction = &bb_drawNothing;
            // entity->drawFunction = &bb_drawRect;
            break;
        }
        case BB_SPIT:
        {
            bb_spitData_t* sData = heap_caps_calloc(1, sizeof(bb_spitData_t), MALLOC_CAP_SPIRAM);
            sData->lifetime      = 0;
            bb_setData(entity, sData, SPIT_DATA);

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);

            push(others, (void*)GARBOTNIK_FLYING);

            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionSpit};
            push(entity->collisions, (void*)collision);

            entity->updateFunction = &bb_updateSpit;
            entity->drawFunction   = &bb_drawSpit;
            break;
        }
        case BB_DONUT:
        {
            if (!(entity->gameData->tutorialFlags & 0b10000))
            {
                entity->gameData->isPaused = true;
                // set the tutorial flag
                entity->gameData->tutorialFlags |= 0b10000;
                bb_entity_t* ovo = bb_createEntity(entityManager, NO_ANIMATION, true, OVO_TALK, 1,
                                                   entity->gameData->camera.camera.pos.x,
                                                   entity->gameData->camera.camera.pos.y, false, true);

                if (ovo != NULL)
                {
                    bb_dialogueData_t* dData = bb_createDialogueData(3, "Ovo");

                    bb_setCharacterLine(dData, 0, "Ovo", "I want to eat that donut RIGHT NOW!");
                    bb_setCharacterLine(dData, 1, "Ovo", "No. I need to focus. I'll tow it back to the booster.");
                    bb_setCharacterLine(dData, 2, "Ovo",
                                        "When I eat it at home, it will be the be the impulse for my next great wile.");

                    dData->curString     = -1;
                    dData->endDialogueCB = &bb_afterGarbotnikTutorialTalk;
                    bb_setData(ovo, dData, DIALOGUE_DATA);
                }
            }

            entity->halfWidth  = 8 << DECIMAL_BITS;
            entity->halfHeight = 8 << DECIMAL_BITS;
            // Give the donut NJIMEIA PHYSX for when it is tethered.
            bb_physicsData_t* physData  = heap_caps_calloc(1, sizeof(bb_physicsData_t), MALLOC_CAP_SPIRAM);
            physData->bounceNumerator   = 2; // 66% bounce
            physData->bounceDenominator = 3;
            physData->vel.y             = -60;
            physData->vel.x             = 60;
            entity->cacheable           = true;
            bb_setData(entity, physData, PHYSICS_DATA);
            entity->updateFunction = &bb_updatePhysicsObject;
            break;
        }
        case BB_SWADGE:
        {
            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionSwadge};
            push(entity->collisions, (void*)collision);

            // sprites loaded just-in-time
            bb_loadSprite("swadge", 12, 1, &entityManager->sprites[BB_SWADGE]);
            break;
        }
        case BB_PANGO_AND_FRIENDS:
        {
            bb_sprite_t* pfSprite  = bb_loadSprite("pangoFriends", 2, 1, &entityManager->sprites[BB_PANGO_AND_FRIENDS]);
            pfSprite->originX      = 29;
            pfSprite->originY      = -240;
            entity->updateFunction = &bb_updatePangoAndFriends;
            entity->updateFarFunction = &bb_updateFarDestroy;
            break;
        }
        case BB_DIVE_SUMMARY:
        {
            entity->updateFunction    = &bb_updateDiveSummary;
            entity->drawFunction      = &bb_drawDiveSummary;
            entity->updateFarFunction = &bb_updateFarDestroy;
            break;
        }
        case BB_FOOD_CART:
        {
            entity->halfWidth  = 22 << DECIMAL_BITS;
            entity->halfHeight = 18 << DECIMAL_BITS;
            entity->cacheable  = true;

            bb_setData(entity, heap_caps_calloc(1, sizeof(bb_foodCartData_t), MALLOC_CAP_SPIRAM), FOOD_CART_DATA);

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionFoodCart};
            push(entity->collisions, (void*)collision);

            entity->drawFunction = &bb_drawFoodCart;

            // sprites loaded just-in-time
            bb_loadSprite("foodCart", 2, 1, &entityManager->sprites[BB_FOOD_CART]);
            break;
        }
        case BB_WILE:
        {
            bb_wileData_t* wData     = heap_caps_calloc(1, sizeof(bb_wileData_t), MALLOC_CAP_SPIRAM);
            wData->bounceNumerator   = 1;
            wData->bounceDenominator = 4;
            bb_setData(entity, wData, WILE_DATA);

            entity->halfHeight = 6 << DECIMAL_BITS;
            entity->halfWidth  = 6 << DECIMAL_BITS;

            entity->updateFunction = &bb_updateWile;
            entity->drawFunction   = &bb_drawWile;
            break;
        }
        case BB_501KG:
        {
            // just in time loading
            bb_loadSprite("501kg", 1, 1, &entityManager->sprites[BB_501KG]);
            bb_setData(entity, heap_caps_calloc(1, sizeof(bb_501kgData_t), MALLOC_CAP_SPIRAM), BB_501KG_DATA);
            entity->updateFunction = &bb_update501kg;
            entity->drawFunction   = &bb_draw501kg;
            break;
        }
        case BB_EXPLOSION:
        {
            bb_setData(entity, heap_caps_calloc(1, sizeof(bb_explosionData_t), MALLOC_CAP_SPIRAM), EXPLOSION_DATA);
            entity->updateFunction = &bb_updateExplosion;
            entity->drawFunction   = &bb_drawExplosion;
            break;
        }
        case BB_ATMOSPHERIC_ATOMIZER:
        {
            bb_setData(entity, heap_caps_calloc(1, sizeof(bb_atmosphericAtomizerData_t), MALLOC_CAP_SPIRAM),
                       ATMOSPHERIC_ATOMIZER_DATA);
            entity->updateFunction = &bb_updateAtmosphericAtomizer;
            entity->drawFunction   = &bb_drawAtmosphericAtomizer;
            break;
        }
        case BB_DRILL_BOT:
        {
            if (!entityManager->sprites[BB_DRILL_BOT].allocated)
            {
                entityManager->sprites[BB_DRILL_BOT].numFrames = 7;
                entityManager->sprites[BB_DRILL_BOT].frames    = heap_caps_calloc(7, sizeof(wsg_t), MALLOC_CAP_SPIRAM);
                entityManager->sprites[BB_DRILL_BOT].allocated = true;
                entityManager->sprites[BB_DRILL_BOT].originX   = 7;
                entityManager->sprites[BB_DRILL_BOT].originY   = 15;
            }

            // sprites loaded just-in-time
            loadWsgInplace("pa-en-004.wsg", &entityManager->sprites[BB_DRILL_BOT].frames[0], true, bb_decodeSpace,
                           bb_hsd); // falling
            loadWsgInplace("pa-en-008.wsg", &entityManager->sprites[BB_DRILL_BOT].frames[1], true, bb_decodeSpace,
                           bb_hsd); // bouncing
            loadWsgInplace("pa-en-005.wsg", &entityManager->sprites[BB_DRILL_BOT].frames[2], true, bb_decodeSpace,
                           bb_hsd); // drilling down
            loadWsgInplace("pa-en-000.wsg", &entityManager->sprites[BB_DRILL_BOT].frames[3], true, bb_decodeSpace,
                           bb_hsd); // walking right 1
            loadWsgInplace("pa-en-001.wsg", &entityManager->sprites[BB_DRILL_BOT].frames[4], true, bb_decodeSpace,
                           bb_hsd); // walking right 2
            loadWsgInplace("pa-en-002.wsg", &entityManager->sprites[BB_DRILL_BOT].frames[5], true, bb_decodeSpace,
                           bb_hsd); // drilling right 1
            loadWsgInplace("pa-en-003.wsg", &entityManager->sprites[BB_DRILL_BOT].frames[6], true, bb_decodeSpace,
                           bb_hsd); // drilling right 2
            bb_drillBotData_t* dbData = heap_caps_calloc(1, sizeof(bb_drillBotData_t), MALLOC_CAP_SPIRAM);
            dbData->bounceNumerator   = 1;
            dbData->bounceDenominator = 4;
            bb_setData(entity, dbData, DRILL_BOT_DATA);

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            push(others, (void*)BU);
            push(others, (void*)BUG);
            push(others, (void*)BUGG);
            push(others, (void*)BUGGO);
            push(others, (void*)BUGGY);
            push(others, (void*)BUTT);
            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionDrillBot};
            push(entity->collisions, (void*)collision);

            entity->halfHeight     = 8 << DECIMAL_BITS;
            entity->halfWidth      = 8 << DECIMAL_BITS;
            entity->updateFunction = &bb_updateDrillBot;
            entity->drawFunction   = &bb_drawDrillBot;
            break;
        }
        case BB_AMMO_SUPPLY:
        {
            bb_loadSprite("ammo_supply", 1, 1, &entityManager->sprites[BB_AMMO_SUPPLY]);
            entityManager->sprites[BB_AMMO_SUPPLY].originX = 14;
            entityManager->sprites[BB_AMMO_SUPPLY].originY = 26;

            bb_timedPhysicsData_t* pData = heap_caps_calloc(1, sizeof(bb_timedPhysicsData_t), MALLOC_CAP_SPIRAM);
            pData->bounceNumerator       = 1;
            pData->bounceDenominator     = 8;
            bb_setData(entity, pData, PHYSICS_DATA);

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionAmmoSupply};
            push(entity->collisions, (void*)collision);

            entity->halfWidth      = 14 << DECIMAL_BITS;
            entity->halfHeight     = 13 << DECIMAL_BITS;
            entity->updateFunction = &bb_updateTimedPhysicsObject;
            break;
        }
        case BB_PACIFIER:
        {
            bb_loadSprite("pacifier", 1, 1, &entityManager->sprites[BB_PACIFIER]);
            entityManager->sprites[BB_PACIFIER].originX = 14;
            entityManager->sprites[BB_PACIFIER].originY = 14;

            bb_timedPhysicsData_t* pData = heap_caps_calloc(1, sizeof(bb_timedPhysicsData_t), MALLOC_CAP_SPIRAM);
            pData->bounceNumerator       = 1;
            pData->bounceDenominator     = 2;
            bb_setData(entity, pData, PHYSICS_DATA);

            entity->halfWidth      = 10 << DECIMAL_BITS;
            entity->halfHeight     = 13 << DECIMAL_BITS;
            entity->updateFunction = &bb_updatePacifier;
            entity->drawFunction   = &bb_drawPacifier;
            break;
        }
        case BB_SPACE_LASER:
        {
            bb_setData(entity, heap_caps_calloc(1, sizeof(bb_spaceLaserData_t), MALLOC_CAP_SPIRAM), SPACE_LASER_DATA);

            entity->halfWidth      = 4 << DECIMAL_BITS;
            entity->halfHeight     = 2000 << DECIMAL_BITS;
            entity->updateFunction = &bb_updateSpaceLaser;
            entity->drawFunction   = &bb_drawSpaceLaser;

            entity->collisions = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rCollisions");
            list_t* others     = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rOthers");
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision
                = heap_caps_calloc_tag(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM, "rCollision");
            *collision = (bb_collision_t){others, bb_onCollisionSpaceLaserGarbotnik};
            push(entity->collisions, (void*)collision);

            list_t* others2 = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rOthers");
            // Neat trick  where you push the value of a bb_spriteDef_t as the pointer. Then when it pops, cast it
            // instead of deferencing and you're good to go! lists store a pointer, but you can abuse that and store any
            // 32 bits of info you want there, as long as you know how to handle it on the other end
            push(others2, (void*)BU);
            push(others2, (void*)BUG);
            push(others2, (void*)BUGG);
            push(others2, (void*)BUGGO);
            push(others2, (void*)BUGGY);
            push(others2, (void*)BUTT);
            push(others2, (void*)EGG);
            bb_collision_t* collision2
                = heap_caps_calloc_tag(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM, "rCollision");
            *collision2 = (bb_collision_t){others2, bb_onCollisionSpaceLaserBug};
            push(entity->collisions, (void*)collision2);

            break;
        }
        case BB_BRICK_TUTORIAL:
        {
            entity->cacheable    = true;
            entity->halfWidth    = HALF_TILE << DECIMAL_BITS;
            entity->halfHeight   = HALF_TILE << DECIMAL_BITS;
            entity->drawFunction = &bb_drawNothing;

            entity->collisions = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            list_t* others     = heap_caps_calloc(1, sizeof(list_t), MALLOC_CAP_SPIRAM);
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision = heap_caps_calloc(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM);
            *collision                = (bb_collision_t){others, bb_onCollisionBrickTutorial};
            push(entity->collisions, (void*)collision);

            break;
        }
        case BB_GARBOTNIK_UI:
        {
            entity->drawFunction = &bb_drawGarbotnikUI;
            break;
        }
        case BB_QUICKPLAY_CONFIRM:
        {
            entity->updateFunction = &bb_updateQuickplay;
            entity->drawFunction   = &bb_drawQuickplay;
            break;
        }
        case BB_FINAL_BOSS:
        {
            bb_finalBossData_t* fbData = heap_caps_calloc(1, sizeof(bb_finalBossData_t), MALLOC_CAP_SPIRAM);
            fbData->health             = 5500;
            bb_setData(entity, fbData, FINAL_BOSS_DATA);

            entity->halfWidth  = 15 << DECIMAL_BITS;
            entity->halfHeight = 15 << DECIMAL_BITS;

            entity->collisions = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rCollisions");
            list_t* others     = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rOthers");
            push(others, (void*)GARBOTNIK_FLYING);
            bb_collision_t* collision
                = heap_caps_calloc_tag(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM, "rCollision");
            *collision = (bb_collision_t){others, bb_onCollisionSpaceLaserGarbotnik};
            push(entity->collisions, (void*)collision);

            list_t* others2 = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "rOthers");
            push(others2, (void*)HARPOON);
            bb_collision_t* collision2
                = heap_caps_calloc_tag(1, sizeof(bb_collision_t), MALLOC_CAP_SPIRAM, "rCollision");
            *collision2 = (bb_collision_t){others2, bb_onCollisionBoss};
            push(entity->collisions, (void*)collision2);

            // just in time loading
            bb_loadSprite("ovovoBoss", 8, 1, &entityManager->sprites[BB_FINAL_BOSS]);
            entity->updateFunction = &bb_updateFinalBoss;
            entity->drawFunction   = &bb_drawFinalBoss;
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
        ESP_LOGD(BB_TAG, "%d/%d entities ^\n", entityManager->activeEntities, MAX_ENTITIES);
    }

    return entity;
}

void bb_freeEntityManager(bb_entityManager_t* self)
{
    for (int i = 0; i < NUM_SPRITES; i++)
    {
        bb_freeSprite(&self->sprites[i]);
    }
    // free and clear
    heap_caps_free(self->entities);
    heap_caps_free(self->frontEntities);

    bb_entity_t* curEntity;
    while (NULL != (curEntity = pop(self->cachedEntities)))
    {
        bb_destroyEntity(curEntity, false, false);
        heap_caps_free(curEntity);
    }

    heap_caps_free(self->cachedEntities);
}