
//==============================================================================
// Includes
//==============================================================================

#include "gs_entityManager.h"
#include "mainMenu.h"
#include "gs_entity.h"
#include "linked_list.h"
#include "gs_utility.h"

//==============================================================================
// Functions
//==============================================================================
void gs_initializeEntityManager(gs_entityManager_t* entityManager, gs_gameData_t* gameData)
{
    // allocate the linked list for entities
    entityManager->entities = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "entities");

    wsgPaletteReset(&entityManager->palettes[GS_GRAYSCALE_PALETTE]);
    wsgPaletteReset(&entityManager->palettes[GS_SUPERBRIGHT_GRAYSCALE_PALETTE]);
    for (paletteColor_t cur = c000; cur <= c555; cur++)
    {
        uint32_t rgb    = paletteToRGB(cur);
        uint32_t r      = (rgb >> 16) & 0xFF; // Extract red channel
        uint32_t g      = (rgb >> 8) & 0xFF;  // Extract green channel
        uint32_t b      = rgb & 0xFF;         // Extract blue channel
        uint32_t sum    = r + g + b;
        uint32_t bright = sum * 4;
        if (bright > 765)
            bright = 765;
        if (sum > 765)
            sum = 765;

        wsgPaletteSet(&entityManager->palettes[GS_GRAYSCALE_PALETTE], cur, (sum / 153) * 43);
        wsgPaletteSet(&entityManager->palettes[GS_SUPERBRIGHT_GRAYSCALE_PALETTE], cur, (bright / 153) * 43);
    }
}

void gs_loadAsset(cnfsFileIdx_t spriteCnfsIdx, uint8_t num_frames, gs_asset_t* asset)
{
    if (!asset->allocated)
    {
        asset->numFrames = num_frames;
        char tag[40];
        snprintf(tag, sizeof(tag), "cnfsFileIdx_t: %d, frames: %d\n", spriteCnfsIdx, num_frames);
        asset->frames    = heap_caps_calloc_tag(num_frames, sizeof(wsg_t), MALLOC_CAP_SPIRAM, tag);
        asset->allocated = true;
    }

    uint16_t maxW = 0;
    uint16_t maxH = 0;
    for (uint8_t frameIdx = 0; frameIdx < num_frames; frameIdx++)
    {
        wsg_t* wsg = &asset->frames[frameIdx];
        if (0 == wsg->w && 0 == wsg->h)
        {
            loadWsgInplace(spriteCnfsIdx + frameIdx, wsg, true, gs_decodeSpace, gs_hsd);
        }
        if (wsg->w > maxW)
        {
            maxW = wsg->w;
        }
        if (wsg->h > maxH)
        {
            maxH = wsg->h;
        }
    }

    asset->originX = maxW >> 1;
    asset->originY = maxH >> 1;
}

void gs_freeAsset(gs_asset_t* asset)
{
    if (asset->allocated)
    {
        for (uint8_t frame = 0; frame < asset->numFrames; frame++)
        {
            if (asset->frames[frame].w || asset->frames[frame].h)
            {
                freeWsg(&asset->frames[frame]);
            }
        }
        heap_caps_free(asset->frames);
        asset->allocated = false;
    }
}

void gs_freeAllAssets(gs_gameData_t* gameData)
{
    for (uint8_t i = 0; i < NUM_ASSETS; i++)
    {
        gs_freeAsset(&gameData->assets[i]);
    }
}

void gs_updateEntities(gs_entityManager_t* entityManager)
{
    node_t* curNode = entityManager->entities->first;
    while (curNode != NULL)
    {
        gs_entity_t* entity = (gs_entity_t*)curNode->val;
        if (entity->updateFunction != NULL)
        {
            entity->updateFunction(entity);
        }
        if (entityManager->entities->first == NULL) // First may become NULL mid loop if all entities are destroyed.
        {
            return;
        }
        curNode = curNode->next;
    }
}

void gs_drawEntity(gs_entity_t* entity)
{
    if (entity->drawFunction == NULL)
        return;

    if (!entity->paused && (entity->type == GS_LOOPING_ANIMATION || entity->type == GS_ONESHOT_ANIMATION))
    {
        entity->animationTimer++;
        if (entity->animationTimer >= entity->gameFramesPerAnimationFrame)
        {
            entity->animationTimer = 0;
            entity->currentAnimationFrame++;
            if (entity->currentAnimationFrame >= entity->gameData->assets[entity->assetIndex].numFrames)
                entity->currentAnimationFrame = 0;
        }
    }
    entity->drawFunction(entity);
}

void gs_drawEntities(gs_entityManager_t* entityManager)
{
    node_t* curNode = entityManager->entities->first;
    uint16_t idx    = 0;
    while (curNode != NULL)
    {
        gs_entity_t* entity = (gs_entity_t*)curNode->val;
        if (entity->destroyFlag)
        {
            gs_freeData(entity);
            curNode = curNode->next;
            removeIdx(entityManager->entities, idx);
        }
        else
        {
            gs_drawEntity(entity);
            curNode = curNode->next;
            idx++;
        }
    }
}

void gs_freeData(gs_entity_t* entity)
{
    if (entity == NULL)
        return;

    if (entity->data != NULL)
    {
        heap_caps_free(entity->data);
        entity->data = NULL;
    }
}

void gs_destroyAllEntities(gs_entityManager_t* entityManager)
{
    gs_entity_t* curEntity;
    while (NULL != (curEntity = pop(entityManager->entities)))
    {
        gs_freeData(curEntity);
        heap_caps_free(curEntity);
    }
}

/**
 * @brief Create an entity and add it to the entity manager. Returns a pointer to the created entity.
 *
 * @param entityManager The entity manager to add the entity to
 * @param spriteCnfsIdx The sprite cnfs index that may be the first frame of the entity's animation
 * @param numFrames The number of frames in the entity's animation, 1 for no animation
 * @param type The animation type for the entity, NO_ANIMATION, ONESHOT_ANIMATION to play animation once, or
 * LOOPING_ANIMATION
 * @param paused Whether the entity's animation is paused
 * @param AssetIndex An index to the loaded wsgs in the entityManager->assets array
 * @param gameFramesPerAnimationFrame The number of game frames per animation frame. Deliberately ignoring delta time
 * for simplicity.
 * @param x The initial x position of the entity
 * @param y The initial y position of the entity
 * @return A pointer to the created entity, or NULL on failure
 */
gs_entity_t* gs_createEntity(gs_entityManager_t* entityManager, uint8_t numFrames, gs_animationType_t type, bool paused,
                             gs_assetIdx_t assetIndex, uint8_t gameFramesPerAnimationFrame, vec_t pos,
                             gs_gameData_t* gameData)
{
    gs_entity_t* entity = heap_caps_calloc(1, sizeof(gs_entity_t), MALLOC_CAP_SPIRAM);
    if (entity == NULL)
    {
        // Exit to the main menu
        switchToSwadgeMode(&mainMenuMode);
        return NULL;
    }

    entity->type                        = type;
    entity->paused                      = paused;
    entity->assetIndex                  = assetIndex;
    entity->gameFramesPerAnimationFrame = gameFramesPerAnimationFrame;
    entity->pos                         = pos;
    entity->gameData                    = gameData;
    entity->drawFunction                = gs_drawAsset;

    push(entityManager->entities, (void*)entity);
    return entity;
}

gs_entity_t* gs_createEntitySimple(gs_entityManager_t* entityManager, gs_assetIdx_t assetIndex, vec_t pos,
                                   gs_gameData_t* gameData)
{
    gs_entity_t* entity;
    switch (assetIndex)
    {
        default:
        {
            return NULL;
        }
    }
    return entity;
}

void gs_freeEntityManager(gs_entityManager_t* entityManager)
{
    if (entityManager == NULL)
        return;

    gs_destroyAllEntities(entityManager);
    heap_caps_free(entityManager->entities);
    entityManager->entities = NULL;
}