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
    DN_CHARACTER_SELECT_DATA,
    DN_TILE_SELECTOR_DATA,
    DN_PROMPT_TO_SKIP_DATA,
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
    bool destroyFlag; // Entity will be cleanly destroyed after engine updating and before engine drawing.
    vec_t pos;
    dn_animationType_t type;
    bool paused;
    dn_assetIdx_t assetIndex;

    uint16_t animationTimer;
    uint8_t gameFramesPerAnimationFrame;
    uint8_t currentAnimationFrame;
    dn_gameData_t* gameData;
};

typedef struct
{
    uint16_t yOffset;
    int16_t yVel;
    dn_entity_t* unit;     // Pointer to the unit on this tile. NULL if no unit is present.
    dn_entity_t* selector; // Pointer to the tile selector. NULL if no selector is present.
} dn_tileData_t;

typedef struct
{
    dn_tileData_t tiles[DN_BOARD_SIZE][DN_BOARD_SIZE];
    dn_entity_t* p1Units[5]; // Pointers to player 1's units. The first unit is king, the other 4 are pawns. NULL
                             // pointers for captured units.
    dn_entity_t* p2Units[5]; // Pointers to player 2's units. The first unit is king, the other 4 are pawns. NULL
                             // pointers for captured units.
    dn_boardPos_t impactPos; // x and y indices of an impact effect.
} dn_boardData_t;

typedef struct
{
    // char array of instruction text
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
    wsgPalette_t screenOffPalette; // replaces album tile colors with the final track colors. The first index color is
                                   // C255. No action is C344.
    wsgPalette_t screenOnPalette;  // Screen is on. Just a more vibrant variation of screenOff.
    dn_track_t tracks[16];         // Array of action tracks in this album in raster order.
    uint16_t rot;                  // Rotation degrees from 0-359 for drawing.
    uint16_t destRot;              // Destination rotation degrees from 0-359 for drawing.
    bool cornerLightBlinking;
    bool cornerLightOn;
    bool screenIsOn;
    int32_t timer;
} dn_albumData_t;

typedef struct
{
    bool selectDiamondShape[45];
    int8_t selectCharacterIdx;
    int32_t xSelectScrollTimer;
    int16_t xSelectScrollOffset;
} dn_characterSelectData_t;

typedef struct
{
    // A bunch of y positions for lines going up.
    uint8_t lineYs[NUM_SELECTOR_LINES];
    // colors that get used for drawing the lines.
    paletteColor_t colors[3];
    // which chess tile the selector is on
    dn_boardPos_t pos;
} dn_tileSelectorData_t;

typedef struct
{
    bool animatingIntroSlide;
    int16_t yOffset;
    uint8_t selectionIdx;
    int16_t selectionAmounts[2];
    bool playerHasSlidThis;
} dn_promptToSkipData_t;


//==============================================================================
// Prototypes
//==============================================================================
void dn_setData(dn_entity_t* self, void* data, dn_dataType_t dataType);

// main game entities
void dn_drawAsset(dn_entity_t* self);
void dn_drawNothing(dn_entity_t* self);

void dn_updateBoard(dn_entity_t* self);
void dn_drawBoard(dn_entity_t* self);
bool dn_isTileSelectabe(dn_entity_t* board, dn_boardPos_t pos);

void dn_updateCurtain(dn_entity_t* self);
void dn_drawCurtain(dn_entity_t* self);

void dn_drawAlbums(dn_entity_t* self);

vec_t dn_colorToTrackCoords(paletteColor_t color);
dn_twoColors_t dn_trackCoordsToColor(vec_t trackCoords);
void dn_addTrackToAlbum(dn_entity_t* album, vec_t trackCoords, dn_track_t track);
void dn_updateAlbum(dn_entity_t* self);
void dn_updateAlbumThenSpawnPromptToSkip(dn_entity_t* self);
void dn_drawAlbum(dn_entity_t* self);

// characterSelect entities
void dn_updateCharacterSelect(dn_entity_t* self);
void dn_drawCharacterSelect(dn_entity_t* self);

void dn_updateTileSelector(dn_entity_t* self);
void dn_drawTileSelectorBackHalf(dn_entity_t* self, int16_t x, int16_t y);
void dn_drawTileSelectorFrontHalf(dn_entity_t* self, int16_t x, int16_t y);

void dn_drawPlayerTurn(dn_entity_t* self);

void dn_updatePromptToSkip(dn_entity_t* self);
void dn_drawPromptToSkip(dn_entity_t* self);

void dn_drawPit(dn_entity_t* self);
void dn_drawPitForeground(dn_entity_t* self);