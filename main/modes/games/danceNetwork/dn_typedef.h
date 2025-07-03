#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <wsg.h>

#define DN_DECIMAL_BITS 4
#define DN_BOARD_SIZE 5
#define DN_TILE_WIDTH 51
#define DN_TILE_HEIGHT 13
#define NUM_ASSETS     10 // The number of dn_asset_t (last accounted for DN_GROUND_TILE_ASSET)
#define NUM_PALETTES   2 // The number of wsgPalette_t (last accounted for DN_RED_FLOOR_PALETTE)

typedef struct dn_entity_t dn_entity_t;
typedef struct dn_gameData_t dn_gameData_t;

typedef enum __attribute__((packed))
{
    DN_ALPHA_SET,
    DN_CHESS_SET,
} dn_characterSet_t;

typedef enum __attribute__((packed))
{
    DN_ALPHA_DOWN_ASSET,
    DN_ALPHA_ORTHO_ASSET,
    DN_ALPHA_UP_ASSET,
    DN_KING_ASSET,
    DN_KING_SMALL_ASSET,
    DN_PAWN_ASSET,
    DN_PAWN_SMALL_ASSET,
    DN_BUCKET_HAT_DOWN_ASSET,
    DN_BUCKET_HAT_UP_ASSET,
    DN_GROUND_TILE_ASSET,
} dn_assetIdx_t;

typedef enum __attribute__((packed))
{
    DN_ONESHOT_ANIMATION,
    DN_LOOPING_ANIMATION,
    DN_NO_ANIMATION,
} dn_animationType_t;

typedef enum __attribute__((packed))
{
    DN_WHITE_CHESS_PALETTE,
    DN_RED_FLOOR_PALETTE,
} dn_paletteIdx_t;

typedef struct __attribute__((packed))
{
    uint8_t originX; //A point inside the sprite measured in pixels from the top left of the sprite
    uint8_t originY; //A point inside the sprite measured in pixels from the top left of the sprite
    uint8_t numFrames; //Just use 1 for no animation
    wsg_t* frames; // Can hold 1 or more pointers to wsg's
    bool allocated; //Whether the frames are allocated.
} dn_asset_t;

typedef struct  __attribute__((packed))
{
    int8_t x;
    int8_t y;
} dn_boardPos_t;

typedef enum __attribute__((packed))
{
    DN_UP,
    DN_DOWN
} dn_facingDir;

typedef enum __attribute__((packed))
{
    DN_PAWN,
    DN_KING
} dn_unitRank;