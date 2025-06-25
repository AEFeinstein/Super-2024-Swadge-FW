#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <wsg.h>

#define DN_DECIMAL_BITS 4
#define DN_BOARD_SIZE 5
#define DN_TILE_WIDTH 51
#define DN_TILE_HEIGHT 13

typedef struct dn_entity_t dn_entity_t;
typedef struct dn_gameData_t dn_gameData_t;

typedef enum
{
    DN_ALPHA_SET,
    DN_CHESS_SET,
} dn_characterSet_t;

typedef enum
{
    DN_ALPHA_DOWN_ASSET,
    DN_ALPHA_ORTHO_ASSET,
    DN_ALPHA_UP_ASSET,
    DN_KING_ASSET,
    DN_SMALL_ASSET,
    DN_PAWN_ASSET,
    DN_PAWN_SMALL_ASSET,
    DN_BUCKET_HAT_DOWN_ASSET,
    DN_BUCKET_HAT_UP_ASSET,
    DN_GROUND_TILE_ASSET,
} dn_assetIdx_t;

typedef enum
{
    DN_ONESHOT_ANIMATION,
    DN_LOOPING_ANIMATION,
    DN_NO_ANIMATION,
} dn_animationType_t;

typedef enum
{
    DN_WHITE_CHESS_PALETTE,
} dn_paletteIdx_t;

typedef struct
{
    int16_t originX;
    int16_t originY;
    uint8_t numFrames;
    wsg_t* frames; // Can hold 1 or more pointers to wsg's
} dn_asset_t;

typedef struct{
    int8_t x;
    int8_t y;
} dn_boardPos_t;