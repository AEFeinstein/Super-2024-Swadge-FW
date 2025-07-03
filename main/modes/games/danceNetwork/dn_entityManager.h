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
    dn_asset_t assets[NUM_ASSETS];//holds loaded single frame sprites or entire animations of wsgs.
    wsgPalette_t palettes[NUM_PALETTES];//holds the palettes for swapping
    list_t* entities;
} dn_entityManager_t;

//==============================================================================
// Prototypes
//==============================================================================
void dn_initializeEntityManager(dn_entityManager_t* entityManager, dn_gameData_t* gameData);
void dn_loadAsset(cnfsFileIdx_t spriteCnfsIdx, uint8_t num_frames, dn_asset_t* sprite);
void dn_freeAsset(dn_asset_t* sprite);
void dn_updateEntities(dn_entityManager_t* entityManager);
void dn_drawEntity(dn_entity_t* entity);
void dn_drawEntities(dn_entityManager_t* entityManager);
void dn_freeData(dn_entity_t* entity);
void dn_destroyAllEntities(dn_entityManager_t* entityManager);
dn_entity_t* dn_createEntitySpecial(dn_entityManager_t* entityManager, cnfsFileIdx_t spriteCnfsIdx, uint8_t numFrames, dn_animationType_t type, bool paused,
                             dn_assetIdx_t AssetIndex, uint8_t gameFramesPerAnimationFrame, vec_t pos, dn_gameData_t* gameData);
dn_entity_t* dn_createEntitySimple(dn_entityManager_t* entityManager, dn_assetIdx_t AssetIndex, vec_t pos, dn_gameData_t* gameData);
void dn_freeEntityManager(dn_entityManager_t* entityManager);