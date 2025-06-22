#pragma once
//==============================================================================
// Includes
//==============================================================================

#include "danceNetwork.h"
#include "dn_entityManager.h"
#include "dn_sprite.h"
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
typedef void (*dn_drawFunction_t)(dn_entityManager_t* entityManager, rectangle_t* camera, dn_entity_t* self);

//==============================================================================
// Structs
//==============================================================================
struct dn_entity_t
{
    bool active;                              // If true, the entity updates and draws
    void* data;
    dn_dataType_t dataType;
    dn_updateFunction_t updateFunction;       // Only set for entities that need update logic
    dn_updateFarFunction_t updateFarFunction; // Only set for execution when the entity is far from the camera center
    dn_drawFunction_t drawFunction;           // Only set for entities such as Garbotnik that need custom drawing logic
    vec_t pos;
};

typedef struct{
    uint16_t yOffset;
    int16_t yVel;
} dn_tileData_t;

typedef struct
{
    dn_tileData_t tiles[BOARD_SIZE][BOARD_SIZE];
} dn_boardData_t;
