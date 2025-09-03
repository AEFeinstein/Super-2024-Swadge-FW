
//==============================================================================
// Includes
//==============================================================================

#include "dn_entityManager.h"
#include "dn_entity.h"
#include "linked_list.h"
#include "dn_utility.h"

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
    wsgPaletteReset(&entityManager->palettes[DN_PIT_WALL_PALETTE]);
    wsgPaletteSet(&entityManager->palettes[DN_PIT_WALL_PALETTE], c212, c100);
    wsgPaletteReset(&entityManager->palettes[DN_REROLL_PALETTE]);
    wsgPaletteSet(&entityManager->palettes[DN_REROLL_PALETTE], c123, c345); // reroll arrow
    wsgPaletteSet(&entityManager->palettes[DN_REROLL_PALETTE], c444, c555); // top face
    wsgPaletteSet(&entityManager->palettes[DN_REROLL_PALETTE], c222, c333); // side face
    wsgPaletteSet(&entityManager->palettes[DN_REROLL_PALETTE], c100, c200); // reds
    wsgPaletteSet(&entityManager->palettes[DN_REROLL_PALETTE], c200, c300);
    wsgPaletteSet(&entityManager->palettes[DN_REROLL_PALETTE], c300, c500);
    wsgPaletteReset(&entityManager->palettes[DN_P2_ARROW_PALETTE]); // flashy prompt arrow
    wsgPaletteSet(&entityManager->palettes[DN_P2_ARROW_PALETTE], c245, c550);
    wsgPaletteSet(&entityManager->palettes[DN_P2_ARROW_PALETTE], c001, c420);

    wsgPaletteReset(&entityManager->palettes[DN_ATTACK1_FLOOR_PALETTE]);
    dn_setFloorPalette(&entityManager->palettes[DN_ATTACK1_FLOOR_PALETTE], c500);
    wsgPaletteReset(&entityManager->palettes[DN_ATTACK2_FLOOR_PALETTE]);
    dn_setFloorPalette(&entityManager->palettes[DN_ATTACK2_FLOOR_PALETTE], c511);
    wsgPaletteReset(&entityManager->palettes[DN_ATTACK3_FLOOR_PALETTE]);
    dn_setFloorPalette(&entityManager->palettes[DN_ATTACK3_FLOOR_PALETTE], c533);
    wsgPaletteReset(&entityManager->palettes[DN_MOVE1_FLOOR_PALETTE]);
    dn_setFloorPalette(&entityManager->palettes[DN_MOVE1_FLOOR_PALETTE], c005);
    wsgPaletteReset(&entityManager->palettes[DN_MOVE2_FLOOR_PALETTE]);
    dn_setFloorPalette(&entityManager->palettes[DN_MOVE2_FLOOR_PALETTE], c125);
    wsgPaletteReset(&entityManager->palettes[DN_MOVE3_FLOOR_PALETTE]);
    dn_setFloorPalette(&entityManager->palettes[DN_MOVE3_FLOOR_PALETTE], c245);
    wsgPaletteReset(&entityManager->palettes[DN_REMIX1_FLOOR_PALETTE]);
    dn_setFloorPalette(&entityManager->palettes[DN_REMIX1_FLOOR_PALETTE], c304);
    wsgPaletteReset(&entityManager->palettes[DN_REMIX2_FLOOR_PALETTE]);
    dn_setFloorPalette(&entityManager->palettes[DN_REMIX2_FLOOR_PALETTE], c405);
    wsgPaletteReset(&entityManager->palettes[DN_REMIX3_FLOOR_PALETTE]);
    dn_setFloorPalette(&entityManager->palettes[DN_REMIX3_FLOOR_PALETTE], c415);

    wsgPaletteReset(&entityManager->palettes[DN_DICE_NO_ARROW_PALETTE]);
    wsgPaletteSet(&entityManager->palettes[DN_DICE_NO_ARROW_PALETTE], c123, cTransparent);
    wsgPaletteSet(&entityManager->palettes[DN_DICE_NO_ARROW_PALETTE], c444, c555); // top face
    wsgPaletteSet(&entityManager->palettes[DN_DICE_NO_ARROW_PALETTE], c222, c333); // side face
    wsgPaletteSet(&entityManager->palettes[DN_DICE_NO_ARROW_PALETTE], c100, c200); // reds
    wsgPaletteSet(&entityManager->palettes[DN_DICE_NO_ARROW_PALETTE], c200, c300);
    wsgPaletteSet(&entityManager->palettes[DN_DICE_NO_ARROW_PALETTE], c300, c500);

    wsgPaletteReset(&entityManager->palettes[DN_GRAYSCALE_PALETTE]);
    wsgPaletteReset(&entityManager->palettes[DN_SUPERBRIGHT_GRAYSCALE_PALETTE]);
    for (paletteColor_t cur = c000; cur <= c555; cur++)
    {
        uint32_t rgb    = paletteToRGB(cur);
        uint32_t r      = (rgb >> 16) & 0xFF; // Extract red channel
        uint32_t g      = (rgb >> 8) & 0xFF;  // Extract green channel
        uint32_t b      = rgb & 0xFF;         // Extract blue channel
        uint32_t sum    = r + g + b;
        uint32_t bright = CLAMP(sum * 4, 0, 765);

        sum = CLAMP(sum, 0, 765);

        wsgPaletteSet(&entityManager->palettes[DN_GRAYSCALE_PALETTE], cur, (sum / 153) * 43);
        wsgPaletteSet(&entityManager->palettes[DN_SUPERBRIGHT_GRAYSCALE_PALETTE], cur, (bright / 153) * 43);
    }

    wsgPaletteReset(&entityManager->palettes[DN_GREEN_TO_CYAN_PALETTE]);
    wsgPaletteSet(&entityManager->palettes[DN_GREEN_TO_CYAN_PALETTE], c050, c055);
    wsgPaletteReset(&entityManager->palettes[DN_GREEN_TO_YELLOW_PALETTE]);
    wsgPaletteSet(&entityManager->palettes[DN_GREEN_TO_YELLOW_PALETTE], c050, c550);

    dn_setCharacterSetPalette(&gameData->entityManager, gameData->characterSets[0]);
}

void dn_loadAsset(cnfsFileIdx_t spriteCnfsIdx, uint8_t num_frames, dn_asset_t* asset)
{
    if (!asset->allocated)
    {
        asset->numFrames = num_frames;
        char tag[40];
        snprintf(tag, sizeof(tag), "cnfsFileIdx_t: %d, frames: %d\n", spriteCnfsIdx, num_frames);
        asset->frames    = heap_caps_calloc_tag(num_frames, sizeof(wsg_t), MALLOC_CAP_SPIRAM, tag);
        asset->allocated = true;
    }

    for (uint8_t frameIdx = 0; frameIdx < num_frames; frameIdx++)
    {
        wsg_t* wsg = &asset->frames[frameIdx];
        if (0 == wsg->h && 0 == wsg->w)
        {
            loadWsgInplace(spriteCnfsIdx + frameIdx, wsg, true, dn_decodeSpace, dn_hsd);
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

void dn_freeAllAssets(dn_gameData_t* gameData)
{
    for (uint8_t i = 0; i < NUM_ASSETS; i++)
    {
        dn_freeAsset(&gameData->assets[i]);
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
        if (entityManager->entities->first == NULL) // First may become NULL mid loop if all entities are destroyed.
        {
            return;
        }
        curNode = curNode->next;
    }
}

void dn_setCharacterSetPalette(dn_entityManager_t* entityManager, dn_characterSet_t characterSet)
{
    switch (characterSet)
    {
        case DN_ALPHA_SET:
        {
            wsgPaletteReset(&entityManager->palettes[DN_RED_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_RED_FLOOR_PALETTE], c253);

            wsgPaletteReset(&entityManager->palettes[DN_ORANGE_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_ORANGE_FLOOR_PALETTE], c354);

            wsgPaletteReset(&entityManager->palettes[DN_YELLOW_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_YELLOW_FLOOR_PALETTE], c455);

            wsgPaletteReset(&entityManager->palettes[DN_GREEN_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_GREEN_FLOOR_PALETTE], c555);

            wsgPaletteReset(&entityManager->palettes[DN_BLUE_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_BLUE_FLOOR_PALETTE], c454);

            wsgPaletteReset(&entityManager->palettes[DN_PURPLE_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_PURPLE_FLOOR_PALETTE], c352);
            break;
        }
        default:
        {
            wsgPaletteReset(&entityManager->palettes[DN_RED_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_RED_FLOOR_PALETTE], c533);

            wsgPaletteReset(&entityManager->palettes[DN_ORANGE_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_ORANGE_FLOOR_PALETTE], c543);

            wsgPaletteReset(&entityManager->palettes[DN_YELLOW_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_YELLOW_FLOOR_PALETTE], c553);

            wsgPaletteReset(&entityManager->palettes[DN_GREEN_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_GREEN_FLOOR_PALETTE], c353);

            wsgPaletteReset(&entityManager->palettes[DN_BLUE_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_BLUE_FLOOR_PALETTE], c335);

            wsgPaletteReset(&entityManager->palettes[DN_PURPLE_FLOOR_PALETTE]);
            dn_setFloorPalette(&entityManager->palettes[DN_PURPLE_FLOOR_PALETTE], c435);
            break;
        }
    }
}

void dn_drawEntity(dn_entity_t* entity)
{
    if (entity->drawFunction == NULL)
        return;
    entity->drawFunction(entity);
}

void dn_drawEntities(dn_entityManager_t* entityManager)
{
    node_t* curNode = entityManager->entities->first;
    uint16_t idx    = 0;
    while (curNode != NULL)
    {
        dn_entity_t* entity = (dn_entity_t*)curNode->val;
        if (entity->destroyFlag)
        {
            dn_freeData(entity);
            curNode = curNode->next;
            removeIdx(entityManager->entities, idx);
        }
        else
        {
            dn_drawEntity(entity);
            curNode = curNode->next;
            idx++;
        }
    }
}

void dn_freeData(dn_entity_t* entity)
{
    if (entity == NULL)
        return;

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
dn_entity_t* dn_createEntitySpecial(dn_entityManager_t* entityManager, uint8_t numFrames, dn_animationType_t type,
                                    bool paused, dn_assetIdx_t assetIndex, uint8_t gameFramesPerAnimationFrame,
                                    vec_t pos, dn_gameData_t* gameData)
{
    dn_entity_t* entity = heap_caps_calloc(1, sizeof(dn_entity_t), MALLOC_CAP_SPIRAM);
    if (entity == NULL)
    {
        return NULL;
    }

    entity->type                        = type;
    entity->paused                      = paused;
    entity->assetIndex                  = assetIndex;
    entity->gameFramesPerAnimationFrame = gameFramesPerAnimationFrame;
    entity->pos                         = pos;
    entity->gameData                    = gameData;
    entity->drawFunction                = dn_drawAsset;

    push(entityManager->entities, (void*)entity);
    return entity;
}

dn_entity_t* dn_createEntitySimple(dn_entityManager_t* entityManager, dn_assetIdx_t assetIndex, vec_t pos,
                                   dn_gameData_t* gameData)
{
    dn_entity_t* entity;
    switch (assetIndex)
    {
        case DN_ALPHA_DOWN_ASSET:
        case DN_ALPHA_ORTHO_ASSET:
        case DN_ALPHA_UP_ASSET:
        case DN_KING_ASSET:
        case DN_KING_SMALL_ASSET:
        case DN_PAWN_ASSET:
        case DN_PAWN_SMALL_ASSET:
        case DN_BUCKET_HAT_DOWN_ASSET:
        case DN_BUCKET_HAT_UP_ASSET:
        { // pawns
            entity = dn_createEntitySpecial(entityManager, 1, DN_NO_ANIMATION, true, assetIndex, 0, pos, gameData);
            entity->data         = heap_caps_calloc(1, sizeof(dn_unitData_t), MALLOC_CAP_SPIRAM);
            entity->dataType     = DN_UNIT_DATA;
            entity->drawFunction = dn_drawNothing; // Drawing of units is handled by dn_drawBoard
            entity->paused       = true;
            break;
        }
        case DN_GROUND_TILE_ASSET:
        {
            entity = dn_createEntitySpecial(entityManager, 1, DN_NO_ANIMATION, true, assetIndex, 0, pos, gameData);
            entity->data         = heap_caps_calloc(1, sizeof(dn_boardData_t), MALLOC_CAP_SPIRAM);
            entity->dataType     = DN_BOARD_DATA;
            entity->drawFunction = dn_drawBoard;
            break;
        }
        case DN_CURTAIN_ASSET:
        {
            if (gameData->characterSets[0] == DN_ALPHA_SET || gameData->characterSets[1] == DN_ALPHA_SET)
            {
                dn_loadAsset(DN_ALPHA_ORTHO_WSG, 1, &gameData->assets[DN_ALPHA_ORTHO_ASSET]);
            }
            if (gameData->characterSets[0] == DN_WHITE_CHESS_SET || gameData->characterSets[1] == DN_WHITE_CHESS_SET
                || gameData->characterSets[0] == DN_BLACK_CHESS_SET || gameData->characterSets[1] == DN_BLACK_CHESS_SET)
            {
                dn_loadAsset(DN_CHESS_ORTHO_WSG, 1, &gameData->assets[DN_CHESS_ORTHO_ASSET]);
            }

            entity = dn_createEntitySpecial(entityManager, 1, DN_NO_ANIMATION, true, assetIndex, 0, pos, gameData);
            entity->data = heap_caps_calloc(1, sizeof(dn_curtainData_t), MALLOC_CAP_SPIRAM);
            ((dn_curtainData_t*)entity->data)->separation
                = -900; // Negative numbers serve as a timer counting up. Separation occurs above zero.
            entity->dataType       = DN_CURTAIN_DATA;
            entity->drawFunction   = dn_drawCurtain;
            entity->updateFunction = dn_updateCurtain;
            break;
        }
        case DN_ALBUM_ASSET:
        {
            dn_loadAsset(DN_ALBUM_WSG, 1, &gameData->assets[assetIndex]);
            dn_loadAsset(DN_STATUS_LIGHT_WSG, 1, &gameData->assets[DN_STATUS_LIGHT_ASSET]);
            entity = dn_createEntitySpecial(entityManager, 1, DN_NO_ANIMATION, true, assetIndex, 0, pos, gameData);
            entity->gameFramesPerAnimationFrame = 4;
            entity->data                        = heap_caps_calloc(1, sizeof(dn_albumData_t), MALLOC_CAP_SPIRAM);
            wsgPaletteReset(&((dn_albumData_t*)entity->data)->screenOffPalette);
            wsgPaletteReset(&((dn_albumData_t*)entity->data)->screenOnPalette);
            wsgPaletteReset(&((dn_albumData_t*)entity->data)->screenAttackPalette);
            wsgPaletteSet(&((dn_albumData_t*)entity->data)->screenOnPalette, c122, c233);
            // Set the color of each track to c344 (no action).
            for (int i = 0; i < 16; i++)
            {
                wsgPaletteSet(&((dn_albumData_t*)entity->data)->screenOffPalette, c255 + i, c344);
                wsgPaletteSet(&((dn_albumData_t*)entity->data)->screenOnPalette, c255 + i, c555);
                wsgPaletteSet(&((dn_albumData_t*)entity->data)->screenAttackPalette, c255 + i, cTransparent);
                // Color the upper and left edges shadowed by the cartridge bevel.
                if (i < 4 || i == 8)
                {
                    wsgPaletteSet(&((dn_albumData_t*)entity->data)->screenOffPalette, c255 - 36 + i, c122);
                    wsgPaletteSet(&((dn_albumData_t*)entity->data)->screenOnPalette, c255 - 36 + i, c233);
                }
            }

            entity->dataType       = DN_ALBUM_DATA;
            entity->drawFunction   = dn_drawAlbum;
            entity->updateFunction = dn_updateAlbum;

            break;
        }
        default:
        {
            return NULL;
        }
    }
    return entity;
}

dn_entity_t* dn_createPrompt(dn_entityManager_t* entityManager, vec_t pos, dn_gameData_t* gameData)
{
    dn_entity_t* prompt = dn_createEntitySpecial(entityManager, 0, DN_NO_ANIMATION, true, DN_NO_ASSET, 0,
                                                 (vec_t){0xffff, 0xffff}, gameData);
    prompt->data        = heap_caps_calloc(1, sizeof(dn_promptData_t), MALLOC_CAP_SPIRAM);
    memset(prompt->data, 0, sizeof(dn_promptData_t));
    dn_promptData_t* promptData     = (dn_promptData_t*)prompt->data;
    promptData->animatingIntroSlide = true;
    promptData->yOffset             = 320; // way off screen to allow more time to look at albums.
    prompt->dataType                = DN_PROMPT_DATA;
    prompt->updateFunction          = dn_updatePrompt;
    prompt->drawFunction            = dn_drawPrompt;
    return prompt;
}

void dn_freeEntityManager(dn_entityManager_t* entityManager)
{
    if (entityManager == NULL)
        return;

    dn_destroyAllEntities(entityManager);
    heap_caps_free(entityManager->entities);
    entityManager->entities = NULL;
}