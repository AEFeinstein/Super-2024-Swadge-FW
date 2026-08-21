#pragma once
//==============================================================================
// Includes
//==============================================================================

#include "gossipStone.h"
#include "gs_entityManager.h"
#include "gs_typedef.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    gs_NULL_DATA,
} gs_dataType_t;

//==============================================================================
// Typedefs
//==============================================================================
typedef void (*gs_updateFunction_t)(gs_entity_t* self);
typedef void (*gs_updateFarFunction_t)(gs_entity_t* self);
typedef void (*gs_drawFunction_t)(gs_entity_t* self);

//==============================================================================
// Structs
//==============================================================================
struct gs_entity_t
{
    void* data;
    gs_dataType_t dataType;
    gs_updateFunction_t updateFunction; // Only set for entities that need update logic
    bool flipped;                       // draw flipped
    gs_drawFunction_t drawFunction;     // Only set for entities such as Garbotnik that need custom drawing logic
    bool destroyFlag;                   // Entity will be destroyed after engine updating and before engine drawing.
    vec_t pos;
    gs_animationType_t type;
    bool paused;
    bool gray;
    gs_assetIdx_t assetIndex;
    uint16_t animationTimer;
    uint8_t gameFramesPerAnimationFrame;
    uint8_t currentAnimationFrame;
    gs_gameData_t* gameData;
};

//==============================================================================
// Prototypes
//==============================================================================
void gs_setData(gs_entity_t* self, void* data, gs_dataType_t dataType);
gs_entity_t* gs_findLastEntityOfType(gs_entity_t* self, gs_dataType_t type);
void gs_drawAsset(gs_entity_t* self);
void gs_drawNothing(gs_entity_t* self);

// main game entities
