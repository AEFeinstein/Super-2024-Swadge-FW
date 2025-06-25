
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
    // allocate the linked list for entities
    entityManager->entities = heap_caps_calloc_tag(1, sizeof(list_t), MALLOC_CAP_SPIRAM, "entities");

    // Palette setup
    wsgPaletteReset(&entityManager->palettes[DN_WHITE_CHESS_PALETTE]);
    wsgPaletteSet(&entityManager->palettes[DN_WHITE_CHESS_PALETTE], c000, c210);
    wsgPaletteSet(&entityManager->palettes[DN_WHITE_CHESS_PALETTE], c001, c421);
    wsgPaletteSet(&entityManager->palettes[DN_WHITE_CHESS_PALETTE], c012, c543);
    wsgPaletteSet(&entityManager->palettes[DN_WHITE_CHESS_PALETTE], c123, c554);
    wsgPaletteSet(&entityManager->palettes[DN_WHITE_CHESS_PALETTE], c145, c555);
}

void dn_loadAsset(cnfsFileIdx_t spriteCnfsIdx, uint8_t num_frames, dn_asset_t* asset)
{
    
    if (!asset->allocated)
    {
        asset->numFrames = num_frames;
        char tag[40];
        snprintf(tag, sizeof(tag), "cnfsFileIdx_t: %d, frames: %d\n", spriteCnfsIdx, num_frames);
        asset->frames = heap_caps_calloc_tag(num_frames, sizeof(wsg_t), MALLOC_CAP_SPIRAM, tag);
        asset->allocated = true;
    }

    for (uint8_t i = 0; i < num_frames; i++)
    {
        asset->frames[i] = cnfs_loadWsg(spriteCnfsIdx + i);
        if (asset->frames[i] == NULL)
        {
            //print an error message
            printf("Failed to load WSG for cnfsFileIdx: %d, frame: %d\n", spriteCnfsIdx, i);
            dn_freeAsset(asset);
            return;
        }
    }
}

void dn_freeAsset(dn_asset_t* asset)
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

/**
 * @brief Create an entity and add it to the entity manager. Returns a pointer to the created entity.
 *
 * @param entityManager The entity manager to add the entity to
 * @param spriteCnfsIdx The sprite cnfs index that may be the first frame of the entity's animation
 * @param numFrames The number of frames in the entity's animation, 1 for no animation
 * @param type The animation type for the entity, NO_ANIMATION, ONESHOT_ANIMATION to play animation once, or LOOPING_ANIMATION
 * @param paused Whether the entity's animation is paused
 * @param AssetIndex An index to the loaded wsgs in the entityManager->assets array
 * @param gameFramesPerAnimationFrame The number of game frames per animation frame. Deliberately ignoring delta time for simplicity.
 * @param x The initial x position of the entity
 * @param y The initial y position of the entity
 * @return A pointer to the created entity, or NULL on failure
 */
dn_entity_t* dn_createEntitySpecial(dn_entityManager_t* entityManager, cnfsFileIdx_t spriteCnfsIdx, uint8_t numFrames, dn_animationType_t type, bool paused,
                             dn_AssetIdx_t AssetIndex, uint8_t gameFramesPerAnimationFrame, uint32_t x, uint32_t y)
{
    dn_entity_t* entity = heap_caps_calloc(1, sizeof(dn_entity_t), MALLOC_CAP_SPIRAM);
    if (entity == NULL)
    {
        return NULL;
    }

    entity->type                       = type;
    entity->paused                     = paused;
    entity->spriteIndex                = spriteIndex;
    entity->gameFramesPerAnimationFrame = gameFramesPerAnimationFrame;
    entity->pos.x                      = x;
    entity->pos.y                      = y;

    push(entityManager->entities, (void*)entity);
    return entity;
}

dn_entity_t* dn_createEntitySimple(dn_entityManager_t* entityManager, dn_AssetIdx_t AssetIndex, uint32_t x, uint32_t y)
{
    switch(AssetIndex)
    {
        case DN_GROUND_TILE_ASSET:
            return dn_createEntitySpecial(entityManager, DN_GROUND_TILE_WSG, 1, NO_ANIMATION, true, spriteIndex, 1, x, y);
        case ALPHA_ORTHO:
            return dn_createEntitySpecial(entityManager, DN_ALPHA_ORTHO_WSG, 1, NO_ANIMATION, true, spriteIndex, 1, x, y);
        case WHITE_CHESS_ORTHO:
            return dn_createEntitySpecial(entityManager, DN_WHITE_KING_WSG, 1, NO_ANIMATION, true, spriteIndex, 1, x, y);
        case BLACK_CHESS_ORTHO:
            return dn_createEntitySpecial(entityManager, DN_BLACK_KING_WSG, 1, NO_ANIMATION, true, spriteIndex, 1, x, y);
        default:
            return NULL;
    }
}


void dn_freeEntityManager(dn_entityManager_t* entityManager)
{
    if (entityManager == NULL) return;

    dn_destroyAllEntities(entityManager);
    heap_caps_free(entityManager->entities);
    entityManager->entities = NULL;
}