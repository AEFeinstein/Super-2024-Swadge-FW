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
    GS_TILEMAP_DATA,
    GS_GOSSIP_DATA,
    GS_FLAME_DATA,
    GS_GOSSIP_STONE_DATA,
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
    gs_paletteIdx_t palleteIdx;
    gs_assetIdx_t assetIndex;
    uint16_t animationTimer;
    uint8_t gameFramesPerAnimationFrame;
    uint8_t currentAnimationFrame;
    gs_gameData_t* gameData;
};

struct gs_tileInfo_t // child class
{
    uint16_t pos;      // bitpacked x,y,z
                       // x is bits 0 through 6
                       // y is bits 7 through 14
                       // z is bit 15
                       // x and y are indices in the tilemap. z is true for foreground false for midground.
    int8_t framePlus1; // 0 is air, 1-12 are zodiac signs, 13 is floor, 14 is wall, 15 is midground wall
};

struct gs_tilemap_t
{
    gs_tileInfo_t* tiles[TILE_FIELD_WIDTH]; ///< The array of tiles.
};

struct gs_hitInfo_t
{
    bool hit;
    vec_t pos; // the precise hit point somewhere on an edge of a tile.
    vec_t normal;
    int32_t tile_i;
    int32_t tile_j;
};

typedef struct
{
    char** messageList;
    uint16_t index;    // The message being displayed
    uint16_t arr_size; // The size of the message list
    uint16_t progress; // From 0 to TYPING_FRAMES the words are typing. If it is TYPING_FRAMES, then shakes are no
                       // longer ignored.
    gs_entity_t* gossipStone; // Reference to make it start and stop animating.
} gs_gossip_t;

typedef struct // parent class
{
    vec_t vel;               // velocity
    uint8_t bounceNumerator; // numerator and denominator are used to control bounciness. 1/1 reflects velocity with the
                             // same magnitude. 1/4 absorbs 75% velocity on a bounce. 2/1 would be looney toons physics.
    uint8_t bounceDenominator;
} gs_physics_t;

typedef struct // child class
{
    vec_t vel;
    uint8_t bounceNumerator; // numerator and denominator are used to control bounciness. 1/1 reflects velocity with the
                             // same magnitude. 1/4 absorbs 75% velocity on a bounce. 2/1 would be looney toons physics.
    uint8_t bounceDenominator;
    // child data
    vec_t accel;       // acceleration
    vec_t previousPos; // position from the previous frame
    bool grounded;
    int32_t rotateDeg;
    int16_t angVel; // angular velocity
    gs_entity_t* flame;
    int32_t fuel; // Remaining fuel.
} gs_gossipStone_t;

typedef struct
{
    int32_t rotateDeg;
} gs_flame_t;

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
void gs_drawFlame(gs_entity_t* self);
void gs_updatePhysicsObject(gs_entity_t* self);
void gs_drawTileMap(gs_entity_t* self);
void gs_collisionCheck(gs_entity_t* tilemap, gs_entity_t* ent, gs_hitInfo_t* hitInfo);
void gs_updateGossipStone(gs_entity_t* self);
