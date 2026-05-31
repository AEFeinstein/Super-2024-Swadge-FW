#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "dn_typedef.h"

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsgPalette_t palettes[NUM_PALETTES]; // holds the palettes for swapping
    list_t* entities;
    dn_entity_t* board;  // a pointer to the game board.
    dn_entity_t* albums; // a pointer to the albums.
} dn_entityManager_t;

//==============================================================================
// Prototypes
//==============================================================================
void dn_initializeEntityManager(dn_entityManager_t* entityManager, dn_gameData_t* gameData);
void dn_loadAsset(cnfsFileIdx_t spriteCnfsIdx, uint8_t num_frames, dn_asset_t* asset);
void dn_freeAsset(dn_asset_t* asset);
void dn_freeAllAssets(dn_gameData_t* gameData);
void dn_updateEntities(dn_entityManager_t* entityManager);
void dn_getScreenPos(dn_entity_t* entity);
void dn_setCharacterSetPalette(dn_entityManager_t* entityManager, dn_characterSet_t characterSet);
void dn_drawEntity(dn_entity_t* entity);
void dn_drawEntities(dn_entityManager_t* entityManager);
void dn_freeData(dn_entity_t* entity);
void dn_destroyAllEntities(dn_entityManager_t* entityManager);
dn_entity_t* dn_createEntitySpecial(dn_entityManager_t* entityManager, uint8_t numFrames, dn_animationType_t type,
                                    bool paused, dn_assetIdx_t AssetIndex, uint8_t gameFramesPerAnimationFrame,
                                    vec_t pos, dn_gameData_t* gameData);
dn_entity_t* dn_createEntitySimple(dn_entityManager_t* entityManager, dn_assetIdx_t AssetIndex, vec_t pos,
                                   dn_gameData_t* gameData);
dn_entity_t* dn_createPrompt(dn_entityManager_t* entityManager, vec_t pos, dn_gameData_t* gameData);
void dn_freeEntityManager(dn_entityManager_t* entityManager);