#pragma once
//==============================================================================
// Includes
//==============================================================================

#include "gossipStone.h"
#include "gs_entityManager.h"
#include "gs_typedef.h"

// how many game frames elapse for each character to type on screen
#define FRAMES_PER_CHAR 2

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    GS_NULL_DATA,
    GS_GOSSIP_DATA,
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

typedef struct
{
    char** messageList;
    uint16_t index;    // The message being displayed
    uint16_t arr_size; // The size of the message list
    uint16_t progress; // From 0 to TYPING_FRAMES the words are typing. If it is TYPING_FRAMES, then shakes are no
                       // longer ignored.
    gs_entity_t* gossipStone; // Reference to make it start and stop animating.
    bool gossipTracking;      // true if new messages count toward the prophecy trophy.
} gs_gossip_t;

//==============================================================================
// Prototypes
//==============================================================================
void gs_setData(gs_entity_t* self, void* data, gs_dataType_t dataType);
gs_entity_t* gs_findLastEntityOfType(gs_entity_t* self, gs_dataType_t type);
void gs_drawAsset(gs_entity_t* self);
void gs_drawNothing(gs_entity_t* self);
// GS entities
void gs_drawSkyGradient(gs_entity_t* self);
void gs_updateGossip(gs_entity_t* self);
void gs_recordProgress(gs_entity_t* self);
void gs_drawGossip(gs_entity_t* self);
