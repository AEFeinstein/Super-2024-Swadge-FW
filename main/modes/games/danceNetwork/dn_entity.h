#pragma once
//==============================================================================
// Includes
//==============================================================================

#include "danceNetwork.h"
#include "dn_entityManager.h"
#include "dn_typedef.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    NULL_DATA,
    BOARD_DATA,
} dn_dataType_t;

//==============================================================================
// Typedefs
//==============================================================================
typedef void (*dn_updateFunction_t)(dn_entity_t* self);
typedef void (*dn_updateFarFunction_t)(dn_entity_t* self);
typedef void (*dn_drawFunction_t)(dn_entity_t* self);

//==============================================================================
// Structs
//==============================================================================
struct dn_entity_t
{
    void* data;
    dn_dataType_t dataType;
    dn_updateFunction_t updateFunction;       // Only set for entities that need update logic
    dn_updateFarFunction_t updateFarFunction; // Only set for execution when the entity is far from the camera center
    dn_drawFunction_t drawFunction;           // Only set for entities such as Garbotnik that need custom drawing logic
    vec_t pos;
    dn_animationType_t type;
    bool paused;
    dn_assetIdx_t assetIndex;
    uint8_t gameFramesPerAnimationFrame;
    dn_gameData_t* gameData;
};

typedef struct{
    uint16_t yOffset;
    int16_t yVel;
} dn_tileData_t;

typedef struct
{
    dn_tileData_t tiles[DN_BOARD_SIZE][DN_BOARD_SIZE];
    dn_entity_t* p1Units[5];//Pointers to player 1's units. The first unit is king, the other 4 are pawns. NULL pointers for captured units.
    dn_entity_t* p2Units[5];//Pointers to player 2's units. The first unit is king, the other 4 are pawns. NULL pointers for captured units.
    dn_boardPos_t impactPos;//x and y indices of an impact effect.
} dn_boardData_t;

//==============================================================================
// Prototypes
//==============================================================================
void dn_setData(dn_entity_t* self, void* data, dn_dataType_t dataType);

void dn_updateBoard(dn_entity_t* self);

void dn_drawBoard(dn_entity_t* self);