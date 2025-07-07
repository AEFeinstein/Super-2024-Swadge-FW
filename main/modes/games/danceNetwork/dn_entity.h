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
    DN_NULL_DATA,
    DN_BOARD_DATA,
    DN_INSTRUCTION_DATA,
    DN_CURTAIN_DATA,
    DN_ALBUMS_DATA,
    DN_ALBUM_DATA,
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
    bool destroyFlag;                         // Entity will be cleanly destroyed after engine updating and before engine drawing.
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
    dn_entity_t* unit; //Pointer to the unit on this tile. NULL if no unit is present.
} dn_tileData_t;

typedef struct
{
    dn_tileData_t tiles[DN_BOARD_SIZE][DN_BOARD_SIZE];
    dn_entity_t* p1Units[5];//Pointers to player 1's units. The first unit is king, the other 4 are pawns. NULL pointers for captured units.
    dn_entity_t* p2Units[5];//Pointers to player 2's units. The first unit is king, the other 4 are pawns. NULL pointers for captured units.
    dn_boardPos_t impactPos;//x and y indices of an impact effect.
} dn_boardData_t;

typedef struct
{
    //char array of instruction text
    char instructions[120];
} dn_instructionData_t;

typedef struct
{
    int16_t separation; // The distance between the curtain and the center of the screen
} dn_curtainData_t;

typedef struct
{
    dn_entity_t* p1Album;
    dn_entity_t* creativeCommonsAlbum;
    dn_entity_t* p2Album;
} dn_albumsData_t;

typedef struct
{
    wsgPalette_t tracksPalette; // replaces album tile colors with the final track colors. The first index color is C255. No action is C344.
    dn_track_t tracks[16]; // Array of action tracks in this album in raster order.
    uint16_t rot; // Rotation degrees from 0-359 for drawing.
} dn_albumData_t;

//==============================================================================
// Prototypes
//==============================================================================
void dn_setData(dn_entity_t* self, void* data, dn_dataType_t dataType);

void dn_updateBoard(dn_entity_t* self);
void dn_drawBoard(dn_entity_t* self);

void dn_updateCurtain(dn_entity_t* self);
void dn_drawCurtain(dn_entity_t* self);

void dn_drawAlbums(dn_entity_t* self);

void dn_drawAlbum(dn_entity_t* self);