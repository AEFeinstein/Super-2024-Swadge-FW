#ifndef PLATFORMER_COMMON_TYPEDEF_INCLUDED
#define PLATFORMER_COMMON_TYPEDEF_INCLUDED

#include "cnfs.h"
#include "vector2d.h"

//==============================================================================
// Constants
//==============================================================================

#define SUBPIXEL_RESOLUTION        4
#define MG_TILESIZE_IN_POWERS_OF_2 4
#define MG_TILESIZE                16
#define MG_HALF_TILESIZE           8
#define DESPAWN_THRESHOLD          64

static const cnfsFileIdx_t MG_BGMS[]
    = {BGM_DE_MAGIO_MID, BGM_SMOOTH_MID, BGM_UNDERGROUND_MID, BGM_CASTLE_MID, BGM_NAME_ENTRY_MID};

//==============================================================================
// Macros
//==============================================================================

#define SIGNOF(x)           ((x > 0) - (x < 0))
#define MG_TO_TILECOORDS(x) ((x) >> MG_TILESIZE_IN_POWERS_OF_2)
#define TO_PIXEL_COORDS(x) ((x) >> SUBPIXEL_RESOLUTION)
#define TO_SUBPIXEL_COORDS(x) ((x) << SUBPIXEL_RESOLUTION)

typedef struct platformer_t platformer_t;
typedef struct mgEntityManager_t mgEntityManager_t;
typedef struct mgTilemap_t mgTilemap_t;
typedef struct mgEntity_t mgEntity_t;

typedef struct {
    int32_t size;
    vec_t collisionPoints[];
} mg_EntityTileCollisionPointList_t;

typedef struct {
   const mg_EntityTileCollisionPointList_t* bottomEdge;
   const mg_EntityTileCollisionPointList_t* topEdge;
   const mg_EntityTileCollisionPointList_t* rightEdge;
   const mg_EntityTileCollisionPointList_t* leftEdge;
} mg_EntityTileCollider_t;

typedef enum
{
    MG_ST_NULL,
    MG_ST_TITLE_SCREEN,
    MG_ST_READY_SCREEN,
    MG_ST_GAME,
    MG_ST_DEAD,
    MG_ST_LEVEL_CLEAR,
    MG_ST_WORLD_CLEAR,
    MG_ST_GAME_CLEAR,
    MG_ST_GAME_OVER,
    MG_ST_HIGH_SCORE_ENTRY,
    MG_ST_HIGH_SCORE_TABLE,
    MG_ST_PAUSE
} mg_gameStateEnum_t;

typedef enum
{
    MG_BGM_NO_CHANGE = -1,
    MG_BGM_NULL,
    MG_BGM_MAIN,
    MG_BGM_ATHLETIC,
    MG_BGM_UNDERGROUND,
    MG_BGM_FORTRESS,
    MG_BGM_NAME_ENTRY
} mg_bgmEnum_t;

typedef enum
{
    MG_SP_PLAYER_IDLE,
    MG_SP_PLAYER_WALK1,
    MG_SP_PLAYER_WALK2,
    MG_SP_PLAYER_WALK3,
    MG_SP_PLAYER_WALK4,
    MG_SP_PLAYER_WALK5,
    MG_SP_PLAYER_WALK6,
    MG_SP_PLAYER_WALK7,
    MG_SP_PLAYER_WALK8,
    MG_SP_PLAYER_WALK9,
    MG_SP_PLAYER_WALK10,
    MG_SP_PLAYER_JUMP,
    MG_SP_PLAYER_JUMP1,
    MG_SP_PLAYER_JUMP2,
    MG_SP_PLAYER_JUMP3,
    MG_SP_PLAYER_JUMP4,
    MG_SP_PLAYER_SLIDE,
    MG_SP_PLAYER_HURT,
    MG_SP_PLAYER_CLIMB,
    MG_SP_PLAYER_WIN,
    MG_SP_ENEMY_BASIC,
    MG_SP_HITBLOCK_CONTAINER,
    MG_SP_HITBLOCK_BRICKS,
    MG_SP_DUSTBUNNY_IDLE,
    MG_SP_DUSTBUNNY_CHARGE,
    MG_SP_DUSTBUNNY_JUMP,
    MG_SP_GAMING_1,
    MG_SP_GAMING_2,
    MG_SP_GAMING_3,
    MG_SP_MUSIC_1,
    MG_SP_MUSIC_2,
    MG_SP_MUSIC_3,
    MG_SP_WARP_1,
    MG_SP_WARP_2,
    MG_SP_WARP_3,
    MG_SP_WASP_1,
    MG_SP_WASP_2,
    MG_SP_WASP_DIVE,
    MG_SP_1UP_1,
    MG_SP_1UP_2,
    MG_SP_1UP_3,
    MG_SP_WAVEBALL_1,
    MG_SP_WAVEBALL_2,
    MG_SP_WAVEBALL_3,
    MG_SP_ENEMY_BUSH_L2,
    MG_SP_ENEMY_BUSH_L3,
    MG_SP_DUSTBUNNY_L2_IDLE,
    MG_SP_DUSTBUNNY_L2_CHARGE,
    MG_SP_DUSTBUNNY_L2_JUMP,
    MG_SP_DUSTBUNNY_L3_IDLE,
    MG_SP_DUSTBUNNY_L3_CHARGE,
    MG_SP_DUSTBUNNY_L3_JUMP,
    MG_SP_WASP_L2_1,
    MG_SP_WASP_L2_2,
    MG_SP_WASP_L2_DIVE,
    MG_SP_WASP_L3_1,
    MG_SP_WASP_L3_2,
    MG_SP_WASP_L3_DIVE,
    MG_SP_CHECKPOINT_INACTIVE,
    MG_SP_CHECKPOINT_ACTIVE_1,
    MG_SP_CHECKPOINT_ACTIVE_2,
    MG_SP_BOUNCE_BLOCK
} mg_spriteDef_t;

typedef enum
{
    MG_TILE_EMPTY,
    MG_TILE_WARP_0,
    MG_TILE_WARP_1,
    MG_TILE_WARP_2,
    MG_TILE_WARP_3,
    MG_TILE_WARP_4,
    MG_TILE_WARP_5,
    MG_TILE_WARP_6,
    MG_TILE_WARP_7,
    MG_TILE_WARP_8,
    MG_TILE_WARP_9,
    MG_TILE_WARP_A,
    MG_TILE_WARP_B,
    MG_TILE_WARP_C,
    MG_TILE_WARP_D,
    MG_TILE_WARP_E,
    MG_TILE_WARP_F,
    MG_TILE_CTNR_COIN,
    MG_TILE_CTNR_10COIN,
    MG_TILE_CTNR_POW1,
    MG_TILE_CTNR_POW2,
    MG_TILE_CTNR_LADDER,
    MG_TILE_CTNR_1UP,
    MG_TILE_CTRL_LEFT,
    MG_TILE_CTRL_RIGHT,
    MG_TILE_CTRL_UP,
    MG_TILE_CTRL_DOWN,
    MG_TILE_UNUSED_27,
    MG_TILE_UNUSED_28,
    MG_TILE_UNUSED_29,
    MG_TILE_INVISIBLE_BLOCK,
    MG_TILE_INVISIBLE_CONTAINER,
    MG_TILE_GRASS,
    MG_TILE_GROUND,
    MG_TILE_BRICK_BLOCK,
    MG_TILE_BLOCK,
    MG_TILE_METAL_BLOCK,
    MG_TILE_METAL_PIPE_H,
    MG_TILE_METAL_PIPE_V,
    MG_TILE_BOUNCE_BLOCK,
    MG_TILE_DIRT_PATH,
    MG_TILEGIRDER,
    MG_TILE_SOLID_UNUSED_42,
    MG_TILE_SOLID_UNUSED_43,
    MG_TILE_SOLID_UNUSED_44,
    MG_TILE_SOLID_UNUSED_45,
    MG_TILE_SOLID_UNUSED_46,
    MG_TILE_SOLID_UNUSED_47,
    MG_TILE_SOLID_UNUSED_48,
    MG_TILE_SOLID_UNUSED_49,
    MG_TILE_SOLID_UNUSED_50,
    MG_TILE_SOLID_UNUSED_51,
    MG_TILE_SOLID_UNUSED_52,
    MG_TILE_SOLID_UNUSED_53,
    MG_TILE_SOLID_UNUSED_54,
    MG_TILE_SOLID_UNUSED_55,
    MG_TILE_SOLID_UNUSED_56,
    MG_TILE_SOLID_UNUSED_57,
    MG_TILE_SOLID_UNUSED_58,
    MG_TILE_GOAL_100PTS,
    MG_TILE_GOAL_500PTS,
    MG_TILE_GOAL_1000PTS,
    MG_TILE_GOAL_2000PTS,
    MG_TILE_GOAL_5000PTS,
    MG_TILE_CONTAINER_1,
    MG_TILE_CONTAINER_2,
    MG_TILE_CONTAINER_3,
    MG_TILE_COIN_1,
    MG_TILE_COIN_2,
    MG_TILE_COIN_3,
    MG_TILE_LADDER,
    MG_TILE_NONSOLID_INTERACTIVE_VISIBLE_71,
    MG_TILE_NONSOLID_INTERACTIVE_VISIBLE_72,
    MG_TILE_NONSOLID_INTERACTIVE_VISIBLE_73,
    MG_TILE_NONSOLID_INTERACTIVE_VISIBLE_74,
    MG_TILE_NONSOLID_INTERACTIVE_VISIBLE_75,
    MG_TILE_NONSOLID_INTERACTIVE_VISIBLE_76,
    MG_TILE_NONSOLID_INTERACTIVE_VISIBLE_77,
    MG_TILE_NONSOLID_INTERACTIVE_VISIBLE_78,
    MG_TILE_NONSOLID_INTERACTIVE_VISIBLE_79,
    MG_TILE_BG_GOAL_ZONE,
    MG_TILE_BG_ARROW_L,
    MG_TILE_BG_ARROW_R,
    MG_TILE_BG_ARROW_U,
    MG_TILE_BG_ARROW_D,
    MG_TILE_BG_ARROW_LU,
    MG_TILE_BG_ARROW_RU,
    MG_TILE_BG_ARROW_LD,
    MG_TILE_BG_ARROW_RD,
    MG_TILE_BG_CLOUD_LD,
    MG_TILE_BG_CLOUD_M,
    MG_TILE_BG_CLOUD_RD,
    MG_TILE_BG_CLOUD_LU,
    MG_TILE_BG_CLOUD_RU,
    MG_TILE_BG_CLOUD_D,
    MG_TILE_BG_CLOUD,
    MG_TILE_BG_TALL_GRASS,
    MG_TILE_BG_MOUNTAIN_L,
    MG_TILE_BG_MOUNTAIN_U,
    MG_TILE_BG_MOUNTAIN_R,
    MG_TILE_BG_MOUNTAIN,
    MG_TILE_BG_METAL,
    MG_TILE_BG_CHAINS,
    MG_TILE_BG_WALL
} mg_tileIndex_t;

typedef enum {
    MG_WSG_PLAYER_IDLE,
    MG_WSG_PLAYER_WALK1,
    MG_WSG_PLAYER_WALK2,
    MG_WSG_PLAYER_WALK3,
    MG_WSG_PLAYER_WALK4,
    MG_WSG_PLAYER_WALK5,
    MG_WSG_PLAYER_WALK6,
    MG_WSG_PLAYER_WALK7,
    MG_WSG_PLAYER_WALK8,
    MG_WSG_PLAYER_WALK9,
    MG_WSG_PLAYER_WALK10,
    MG_WSG_PLAYER_JUMP,
    MG_WSG_PLAYER_JUMP1,
    MG_WSG_PLAYER_JUMP2,
    MG_WSG_PLAYER_JUMP3,
    MG_WSG_PLAYER_JUMP4,
    
    MG_WSG_PLAYER_SHOOT_IDLE,
    MG_WSG_PLAYER_SHOOT_WALK1,
    MG_WSG_PLAYER_SHOOT_WALK2,
    MG_WSG_PLAYER_SHOOT_WALK3,
    MG_WSG_PLAYER_SHOOT_WALK4,
    MG_WSG_PLAYER_SHOOT_WALK5,
    MG_WSG_PLAYER_SHOOT_WALK6,
    MG_WSG_PLAYER_SHOOT_WALK7,
    MG_WSG_PLAYER_SHOOT_WALK8,
    MG_WSG_PLAYER_SHOOT_WALK9,
    MG_WSG_PLAYER_SHOOT_WALK10,
    MG_WSG_PLAYER_SHOOT_JUMP,
    MG_WSG_PLAYER_SHOOT_JUMP1,
    MG_WSG_PLAYER_SHOOT_JUMP2,
    MG_WSG_PLAYER_SHOOT_JUMP3,
    MG_WSG_PLAYER_SHOOT_JUMP4,

    MG_WSG_PLAYER_SLIDE,
    MG_WSG_PLAYER_HURT,
    MG_WSG_PLAYER_CLIMB,
    MG_WSG_PLAYER_WIN,
    MG_WSG_ENEMY_BASIC,
    MG_WSG_DUSTBUNNY_IDLE,
    MG_WSG_DUSTBUNNY_CHARGE,
    MG_WSG_DUSTBUNNY_JUMP,
    MG_WSG_GAMING_1,
    MG_WSG_GAMING_2,
    MG_WSG_GAMING_3,
    MG_WSG_MUSIC_1,
    MG_WSG_MUSIC_2,
    MG_WSG_MUSIC_3,
    MG_WSG_WARP_1,
    MG_WSG_WARP_2,
    MG_WSG_WARP_3,
    MG_WSG_WASP_1,
    MG_WSG_WASP_2,
    MG_WSG_WASP_DIVE,
    MG_WSG_1UP_1,
    MG_WSG_1UP_2,
    MG_WSG_1UP_3,
    MG_WSG_WAVEBALL_1,
    MG_WSG_WAVEBALL_2,
    MG_WSG_WAVEBALL_3,
    MG_WSG_ENEMY_BUSH_L2,
    MG_WSG_ENEMY_BUSH_L3,
    MG_WSG_DUSTBUNNY_L2_IDLE,
    MG_WSG_DUSTBUNNY_L2_CHARGE,
    MG_WSG_DUSTBUNNY_L2_JUMP,
    MG_WSG_DUSTBUNNY_L3_IDLE,
    MG_WSG_DUSTBUNNY_L3_CHARGE,
    MG_WSG_DUSTBUNNY_L3_JUMP,
    MG_WSG_WASP_L2_1,
    MG_WSG_WASP_L2_2,
    MG_WSG_WASP_L2_DIVE,
    MG_WSG_WASP_L3_1,
    MG_WSG_WASP_L3_2,
    MG_WSG_WASP_L3_DIVE,
    MG_WSG_CHECKPOINT_INACTIVE,
    MG_WSG_CHECKPOINT_ACTIVE_1,
    MG_WSG_CHECKPOINT_ACTIVE_2,

    MG_WSG_GRASS,
    MG_WSG_GROUND,
    MG_WSG_BRICK_BLOCK,
    MG_WSG_BLOCK,
    MG_WSG_METAL_BLOCK,
    MG_WSG_METAL_PIPE_H,
    MG_WSG_METAL_PIPE_V,
    MG_WSG_BOUNCE_BLOCK,
    MG_WSG_DIRT_PATH,
    MG_WSG_GIRDER,
    MG_WSG_GOAL_100PTS,
    MG_WSG_GOAL_500PTS,
    MG_WSG_GOAL_1000PTS,
    MG_WSG_GOAL_2000PTS,
    MG_WSG_GOAL_5000PTS,
    MG_WSG_CONTAINER_1,
    MG_WSG_CONTAINER_2,
    MG_WSG_CONTAINER_3,
    MG_WSG_COIN_1,
    MG_WSG_COIN_2,
    MG_WSG_COIN_3,
    MG_WSG_LADDER,
    MG_WSG_BG_GOAL_ZONE,
    MG_WSG_BG_ARROW_L,
    MG_WSG_BG_ARROW_R,
    MG_WSG_BG_ARROW_U,
    MG_WSG_BG_ARROW_D,
    MG_WSG_BG_ARROW_LU,
    MG_WSG_BG_ARROW_RU,
    MG_WSG_BG_ARROW_LD,
    MG_WSG_BG_ARROW_RD,
    MG_WSG_BG_CLOUD_LD,
    MG_WSG_BG_CLOUD_M,
    MG_WSG_BG_CLOUD_RD,
    MG_WSG_BG_CLOUD_LU,
    MG_WSG_BG_CLOUD_RU,
    MG_WSG_BG_CLOUD_D,
    MG_WSG_BG_CLOUD,
    MG_WSG_BG_TALL_GRASS,
    MG_WSG_BG_MOUNTAIN_L,
    MG_WSG_BG_MOUNTAIN_U,
    MG_WSG_BG_MOUNTAIN_R,
    MG_WSG_BG_MOUNTAIN,
    MG_WSG_BG_METAL,
    MG_WSG_BG_CHAINS,
    MG_WSG_BG_WALL
} mg_wsgIndex_t;


typedef enum
{
    MG_EDGE_NONE,
    MG_EDGE_TOP,
    MG_EDGE_BOTTOM,
    MG_EDGE_TB,
    MG_EDGE_LEFT,
    MG_EDGE_TL,
    MG_EDGE_BL,
    MG_EDGE_BLT,
    MG_EDGE_RIGHT,
    MG_EDGE_TR,
    MG_EDGE_BR,
    MG_EDGE_TBR,
    MG_EDGE_LR,
    MG_EDGE_TLR,
    MG_EDGE_BLR,
    MG_EDGE_TBLR
} mgEdge_t;

/*
static const int MG_1x2_TILE_COLLISION_OFFSETS_IN_PIXELS[]
    = {0, 8, MG_EDGE_BLR, 0, -8, MG_EDGE_TLR};

static const int MG_TILE_COLLISION_OFFSETS_1x2_BOTTOM_EDGE[]
    = {-7, 15, 0, 15, 6, 15};

static const int MG_TILE_COLLISION_OFFSETS_1x2_TOP_EDGE[]
    = {-7, -15, 0, -15, 7, -15};

static const int MG_TILE_COLLISION_OFFSETS_1x2_RIGHT_EDGE[]
    = {8, 14, 8, 0, 8, -14};

static const int MG_TILE_COLLISION_OFFSETS_1x2_LEFT_EDGE[]
    = {-7, 14, -7, 0, -7, -14};
*/

static const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_bottomEdge = 
{
    .collisionPoints = {{
        .x = -7,
        .y = 15
    },
    {
        .x = 0,
        .y = 15
    },
    {
        .x = 6,
        .y = 15
    }},
    .size = 3
};

static const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_topEdge = 
{
    .collisionPoints = {{
        .x = -7,
        .y = -15
    },
    {
        .x = 0,
        .y = -15
    },
    {
        .x = 7,
        .y = -15
    }},
    .size = 3
};

static const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_rightEdge = 
{
    .collisionPoints = {{
        .x = 8,
        .y = 14
    },
    {
        .x = 8,
        .y = 0
    },
    {
        .x = 8,
        .y = -14
    }},
    .size = 3
};

static const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_leftEdge = 
{
    .collisionPoints = {{
        .x = -7,
        .y = 14
    },
    {
        .x = -7,
        .y = 0
    },
    {
        .x = -7,
        .y = -14
    }},
    .size = 3
};

static const mg_EntityTileCollider_t entityTileCollider_1x2 = {
    .bottomEdge = &mgTileCollisionOffsets_1x2_bottomEdge,
    .topEdge = &mgTileCollisionOffsets_1x2_topEdge,
    .rightEdge = &mgTileCollisionOffsets_1x2_rightEdge,
    .leftEdge = &mgTileCollisionOffsets_1x2_leftEdge
};

#endif