#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "swadge.h"
#include "gs_typedef.h"

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
    gs_entity_t* tilemap;
    gs_camera_t camera;
    gs_entity_t* gossip;
    gs_entity_t* gossipStone;
    int8_t zoom;//0 is normal scale, each positive step doubles sprite sizes, negative halves sprites.
} gs_entityManager_t;

//==============================================================================
// Prototypes
//==============================================================================
void gs_initializeEntityManager(gs_entityManager_t* entityManager, gs_gameData_t* gameData);
void gs_loadAsset(cnfsFileIdx_t spriteCnfsIdx, uint8_t num_frames, gs_asset_t* asset);
void gs_freeAsset(gs_asset_t* asset);
void gs_freeAllAssets(gs_gameData_t* gameData);
void gs_updateEntities(gs_entityManager_t* entityManager);
void gs_getScreenPos(gs_entity_t* entity);
void gs_drawEntity(gs_entity_t* entity);
void gs_drawEntities(gs_entityManager_t* entityManager);
void gs_freeData(gs_entity_t* entity);
void gs_destroyAllEntities(gs_entityManager_t* entityManager);
gs_entity_t* gs_createEntity(gs_entityManager_t* entityManager, uint8_t numFrames, gs_animationType_t type, bool paused,
                             gs_assetIdx_t AssetIndex, uint8_t gameFramesPerAnimationFrame, vec_t pos,
                             gs_gameData_t* gameData);
gs_entity_t* gs_createEntityFirst(gs_entityManager_t* entityManager, uint8_t numFrames, gs_animationType_t type, bool paused,
                             gs_assetIdx_t assetIndex, uint8_t gameFramesPerAnimationFrame, vec_t pos,
                             gs_gameData_t* gameData);
gs_entity_t* gs_createEntityBefore(void* before, gs_entityManager_t* entityManager, uint8_t numFrames,
                                   gs_animationType_t type, bool paused, gs_assetIdx_t assetIndex,
                                   uint8_t gameFramesPerAnimationFrame, vec_t pos, gs_gameData_t* gameData);
gs_entity_t* gs_createEntitySimple(gs_entityManager_t* entityManager, gs_assetIdx_t AssetIndex, vec_t pos,
                                   gs_gameData_t* gameData);
void gs_freeEntityManager(gs_entityManager_t* entityManager);