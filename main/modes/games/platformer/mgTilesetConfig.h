#ifndef _MGTILESETCONFIG_H_
#define _MGTILESETCONFIG_H_

//==============================================================================
// Includes
//==============================================================================

#include "mega_pulse_ex_typedef.h"
#include "mgSprite.h"

//==============================================================================
// Constants
//==============================================================================

#define MG_KINETIC_DONUT_TILESET_MAP_LENGTH 99
#define MG_LEVEL_SELECT_TILESET_MAP_LENGTH  20

#define MG_IMAGE_FILENAME_LOOKUP_OFFSET 0
#define MG_WSG_INDEX_LOOKUP_OFFSET      1
#define MG_TILE_INDEX_LOOKUP_OFFSET     2
#define MG_TILESET_MAP_ROW_LENGTH       3

//==============================================================================
// Function Prototypes
//==============================================================================

bool mg_kineticDonutTileset_needsTransparency(uint8_t tileId);
void mg_kineticDonutTileset_animateTiles(uint8_t tileId);

bool mg_levelSelectTileset_needsTransparency(uint8_t tileId);
void mg_levelSelectTileset_animateTiles(uint8_t tileId);

//==============================================================================
// Externs
//==============================================================================

extern const uint16_t mg_kineticDonutTileset[];
extern const mgSprite_t mg_severYatagaBossSpriteMetadataSet[];
extern const uint16_t mg_levelSelectTileset[];
extern const uint16_t mg_smashGorillaTileset[];
extern const mgSprite_t mg_smashGorillaBossSpriteMetadataSet[];
extern const mgSprite_t mg_drainBatBossSpriteMetadataSet[];
extern const mgSprite_t mg_grindPangolinBossSpriteMetadataSet[];
extern const mgSprite_t mg_kineticDonutBossSpriteMetadataSet[];
extern const mgSprite_t mg_flareGryffynBossSpriteMetadataSet[];
extern const mgSprite_t mg_deadeyeChirpziBossSpriteMetadataSet[];
extern const mgSprite_t mg_trashManBossSpriteMetadataSet[];
extern const mgSprite_t mg_bigmaBossSpriteMetadataSet[];
extern const uint16_t mg_severYatagaTileset[];
extern const uint16_t mg_trashManTileset[];
extern const uint16_t mg_grindPangolinTileset[];
extern const uint16_t mg_deadeyeChirpziTileset[];
extern const uint16_t mg_drainBatTileset[];
extern const uint16_t mg_flareGryffynTileset[];
extern const uint16_t mg_bigmaTileset[];

#endif