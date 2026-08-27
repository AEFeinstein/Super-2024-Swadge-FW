#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <wsg.h>

#define DECIMAL_BITS   4
#define NUM_ASSETS        GS_NO_ASSET      // The number of gs_asset_t
#define NUM_PALETTES      GS_COUNT_PALETTE // The number of wsgPalette_t
#define MAX_LERP_AMOUNT   30000
#define TILE_FIELD_WIDTH  120 // matches the level wsg graphic width
#define TILE_FIELD_HEIGHT 120 // matches the level wsg graphic height

typedef struct gs_entity_t gs_entity_t;
typedef struct gs_gameData_t gs_gameData_t;
typedef struct gs_tilemap_t gs_tilemap_t;
typedef struct gs_hitInfo_t gs_hitInfo_t;
typedef struct gs_tileInfo_t gs_tileInfo_t;

typedef void (*gs_callbackFunction_t)(gs_entity_t* self);

typedef enum __attribute__((packed))
{
    GS_SKY_GRADIENT_ASSET,
    GS_HILL_ASSET,
    GS_MOON_ASSET,
    GS_GOSSIP_STONE_ASSET,
    GS_FLAME_ASSET,
    GS_MOON_TILE_ASSET,
    GS_NO_ASSET, // Keep this one at the end of the enum. Used for entities with no wsgs.
} gs_assetIdx_t;

typedef enum __attribute__((packed))
{
    GS_ONESHOT_ANIMATION,
    GS_LOOPING_ANIMATION,
    GS_NO_ANIMATION,
} gs_animationType_t;

typedef enum __attribute__((packed))
{
    GS_UNTOUCHED_PALETTE,
    GS_BLUE_PALETTE,
    GS_RED_PALETTE,
    GS_COUNT_PALETTE, // Keep this at the end to keep count of the number of palettes.
} gs_paletteIdx_t;

typedef struct __attribute__((packed))
{
    uint8_t originX;   // A point inside the sprite measured in pixels from the top left of the sprite
    uint8_t originY;   // A point inside the sprite measured in pixels from the top left of the sprite
    uint8_t numFrames; // Just use 1 for no animation
    wsg_t* frames;     // Can hold 1 or more pointers to wsg's
    bool allocated;    // Whether the frames are allocated.
} gs_asset_t;