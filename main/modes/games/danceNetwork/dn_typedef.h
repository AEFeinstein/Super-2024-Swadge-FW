#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <wsg.h>

#define DN_DECIMAL_BITS    4
#define DN_BOARD_SIZE      5
#define DN_TILE_WIDTH      51
#define DN_TILE_HEIGHT     13
#define NUM_ASSETS         31 // The number of dn_asset_t (last accounted for DN_QR_ASSET)
#define NUM_PALETTES       25 // The number of wsgPalette_t (last accounted for green to yellow)
#define NUM_SELECTOR_LINES 15 // Creates more chaotic lines in the selector graphic

typedef struct dn_entity_t dn_entity_t;
typedef struct dn_gameData_t dn_gameData_t;

typedef void (*dn_callbackFunction_t)(dn_entity_t* self);

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
    DN_CURTAIN_ASSET,
    DN_CHESS_ORTHO_ASSET,
    DN_ALBUM_ASSET,
    DN_STATUS_LIGHT_ASSET,
    DN_SPEAKER_STAND_ASSET,
    DN_SPEAKER_ASSET,
    DN_PIT_ASSET,
    DN_MINI_TILE_ASSET,
    DN_REROLL_ASSET,
    DN_NUMBER_ASSET,
    DN_ALBUM_EXPLOSION_ASSET,
    DN_MMM_UP_ASSET,
    DN_SWAP_ASSET,
    DN_SKIP_ASSET,
    DN_GLITCH_ASSET,
    DN_DANCENONYDA_ASSET,
    DN_TFT_ASSET,
    DN_TEXTBOX_ASSET,
    DN_MMM_NEXT_ASSET,
    DN_MMM_SUBMENU_ASSET,
    DN_QR_ASSET,
    DN_NO_ASSET, // Keep this one at the end of the enum. Used for entities with no wsgs.
} dn_assetIdx_t;

typedef enum __attribute__((packed))
{
    DN_ALPHA_SET,
    DN_BLACK_CHESS_SET,
    DN_WHITE_CHESS_SET,
} dn_characterSet_t;

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
    DN_ORANGE_FLOOR_PALETTE,
    DN_YELLOW_FLOOR_PALETTE,
    DN_GREEN_FLOOR_PALETTE,
    DN_BLUE_FLOOR_PALETTE,
    DN_PURPLE_FLOOR_PALETTE,
    DN_PIT_WALL_PALETTE,
    DN_REROLL_PALETTE,
    DN_P2_ARROW_PALETTE, // up and down arrows for the player 2 prompt
    DN_RED_ATTACK_FLOOR_PALETTE,
    DN_ATTACK1_FLOOR_PALETTE,
    DN_ATTACK2_FLOOR_PALETTE,
    DN_ATTACK3_FLOOR_PALETTE,
    DN_MOVE1_FLOOR_PALETTE,
    DN_MOVE2_FLOOR_PALETTE,
    DN_MOVE3_FLOOR_PALETTE,
    DN_REMIX1_FLOOR_PALETTE,
    DN_REMIX2_FLOOR_PALETTE,
    DN_REMIX3_FLOOR_PALETTE,
    DN_DICE_NO_ARROW_PALETTE,
    DN_GRAYSCALE_PALETTE,
    DN_SUPERBRIGHT_GRAYSCALE_PALETTE,
    DN_GREEN_TO_CYAN_PALETTE,
    DN_GREEN_TO_YELLOW_PALETTE,
} dn_paletteIdx_t;

typedef struct __attribute__((packed))
{
    uint8_t originX;   // A point inside the sprite measured in pixels from the top left of the sprite
    uint8_t originY;   // A point inside the sprite measured in pixels from the top left of the sprite
    uint8_t numFrames; // Just use 1 for no animation
    wsg_t* frames;     // Can hold 1 or more pointers to wsg's
    bool allocated;    // Whether the frames are allocated.
} dn_asset_t;

typedef struct __attribute__((packed))
{
    int8_t x;
    int8_t y;
} dn_boardPos_t;

typedef struct __attribute__((packed))
{
    paletteColor_t lit;
    paletteColor_t unlit;
} dn_twoColors_t;

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

typedef enum __attribute__((packed))
{
    DN_P1_DANCE_PHASE,
    DN_P1_UPGRADE_PHASE,
    DN_P2_DANCE_PHASE,
    DN_P2_UPGRADE_PHASE,
} dn_phase_t;

typedef enum __attribute__((packed))
{
    DN_NONE_TRACK,
    DN_RED_TRACK,
    DN_BLUE_TRACK,
    DN_REMIX_TRACK,
    DN_RED_TRACK_INVALID,
    DN_BLUE_TRACK_INVALID,
    DN_REMIX_TRACK_INVALID,
    DN_UNIT_SELECTION,
} dn_track_t;

typedef struct __attribute__((packed))
{
    dn_boardPos_t pos;
    dn_track_t action;
} dn_action_t;

//remove redundant struct later
typedef struct __attribute__((packed))
{
    char title[31];
    char body[160];
} dn_tutorialPage_t;

