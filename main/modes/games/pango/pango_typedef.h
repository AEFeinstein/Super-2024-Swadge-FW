#ifndef PANGO_COMMON_TYPEDEF_INCLUDED
#define PANGO_COMMON_TYPEDEF_INCLUDED

typedef struct pango_t pango_t;
typedef struct paEntityManager_t paEntityManager_t;
typedef struct paTilemap_t paTilemap_t;
typedef struct paEntity_t paEntity_t;

typedef enum
{
    PA_ST_NULL,
    PA_ST_TITLE_SCREEN,
    PA_ST_READY_SCREEN,
    PA_ST_GAME,
    PA_ST_DEAD,
    PA_ST_LEVEL_CLEAR,
    PA_ST_WORLD_CLEAR,
    PA_ST_GAME_CLEAR,
    PA_ST_GAME_OVER,
    PA_ST_HIGH_SCORE_ENTRY,
    PA_ST_HIGH_SCORE_TABLE,
    PA_ST_PAUSE
} pa_gameStateEnum_t;

typedef enum
{
    PA_BGM_NO_CHANGE,
    PA_BGM_MAIN,
    PA_BGM_ATHLETIC,
    PA_BGM_UNDERGROUND,
    PA_BGM_FORTRESS,
    PA_BGM_NULL
} pa_bgmEnum_t;

typedef enum
{
    PA_SP_PLAYER_SOUTH,
    PA_SP_PLAYER_WALK_SOUTH,
    PA_SP_PLAYER_NORTH,
    PA_SP_PLAYER_WALK_NORTH,
    PA_SP_PLAYER_SIDE,
    PA_SP_PLAYER_WALK_SIDE_1,
    PA_SP_PLAYER_WALK_SIDE_2,
    PA_SP_PLAYER_PUSH_SOUTH_1,
    PA_SP_PLAYER_PUSH_SOUTH_2,
    PA_SP_PLAYER_PUSH_NORTH_1,
    PA_SP_PLAYER_PUSH_NORTH_2,
    PA_SP_PLAYER_PUSH_SIDE_1,
    PA_SP_PLAYER_PUSH_SIDE_2,
    PA_SP_PLAYER_HURT,
    PA_SP_PLAYER_WIN,
    PA_SP_PLAYER_ICON,
    PA_SP_BLOCK,
    PA_SP_BONUS_BLOCK,
    PA_SP_ENEMY_SOUTH,
    PA_SP_ENEMY_NORTH,
    PA_SP_ENEMY_SIDE_1,
    PA_SP_ENEMY_SIDE_2,
    PA_SP_ENEMY_DRILL_SOUTH,
    PA_SP_ENEMY_DRILL_NORTH,
    PA_SP_ENEMY_DRILL_SIDE_1,
    PA_SP_ENEMY_DRILL_SIDE_2,
    PA_SP_ENEMY_STUN,
    PA_SP_BREAK_BLOCK,
    PA_SP_BREAK_BLOCK_1,
    PA_SP_BREAK_BLOCK_2,
    PA_SP_BREAK_BLOCK_3,
    PA_SP_BLOCK_FRAGMENT
} pa_spriteDef_t;

typedef enum
{
    PA_TILE_EMPTY,
    PA_TILE_WALL_0,
    PA_TILE_WALL_1,
    PA_TILE_WALL_2,
    PA_TILE_WALL_3,
    PA_TILE_WALL_4,
    PA_TILE_WALL_5,
    PA_TILE_WALL_6,
    PA_TILE_WALL_7,
    PA_TILE_BLOCK,
    PA_TILE_SPAWN_BLOCK_0,
    PA_TILE_SPAWN_BLOCK_1,
    PA_TILE_SPAWN_BLOCK_2,
    PA_TILE_BONUS_BLOCK_0,
    PA_TILE_BONUS_BLOCK_1,
    PA_TILE_BONUS_BLOCK_2,
    PA_TILE_INVISIBLE_BLOCK
} pa_tileDef_t;

typedef enum
{
    PA_PLAYER_CHARACTER_PANGO,
    PA_PLAYER_CHARACTER_PO,
    PA_PLAYER_CHARACTER_PIXEL,
    PA_PLAYER_CHARACTER_GIRL
} pa_playerCharacterDef_t;

typedef enum
{
    PA_WSG_PANGO_SOUTH,
    PA_WSG_PANGO_WALK_SOUTH,
    PA_WSG_PANGO_NORTH,
    PA_WSG_PANGO_WALK_NORTH,
    PA_WSG_PANGO_SIDE,
    PA_WSG_PANGO_WALK_SIDE_1,
    PA_WSG_PANGO_WALK_SIDE_2,
    PA_WSG_PANGO_PUSH_SOUTH_1,
    PA_WSG_PANGO_PUSH_SOUTH_2,
    PA_WSG_PANGO_PUSH_NORTH_1,
    PA_WSG_PANGO_PUSH_NORTH_2,
    PA_WSG_PANGO_PUSH_SIDE_1,
    PA_WSG_PANGO_PUSH_SIDE_2,
    PA_WSG_PANGO_HURT,
    PA_WSG_PANGO_WIN,
    PA_WSG_PANGO_ICON,
    PA_WSG_PO_SOUTH,
    PA_WSG_PO_WALK_SOUTH,
    PA_WSG_PO_NORTH,
    PA_WSG_PO_WALK_NORTH,
    PA_WSG_PO_SIDE,
    PA_WSG_PO_WALK_SIDE_1,
    PA_WSG_PO_WALK_SIDE_2,
    PA_WSG_PO_PUSH_SOUTH_1,
    PA_WSG_PO_PUSH_SOUTH_2,
    PA_WSG_PO_PUSH_NORTH_1,
    PA_WSG_PO_PUSH_NORTH_2,
    PA_WSG_PO_PUSH_SIDE_1,
    PA_WSG_PO_PUSH_SIDE_2,
    PA_WSG_PO_HURT,
    PA_WSG_PO_WIN,
    PA_WSG_PO_ICON,
    PA_WSG_PIXEL_SOUTH,
    PA_WSG_PIXEL_WALK_SOUTH,
    PA_WSG_PIXEL_NORTH,
    PA_WSG_PIXEL_WALK_NORTH,
    PA_WSG_PIXEL_SIDE,
    PA_WSG_PIXEL_WALK_SIDE_1,
    PA_WSG_PIXEL_WALK_SIDE_2,
    PA_WSG_PIXEL_PUSH_SOUTH_1,
    PA_WSG_PIXEL_PUSH_SOUTH_2,
    PA_WSG_PIXEL_PUSH_NORTH_1,
    PA_WSG_PIXEL_PUSH_NORTH_2,
    PA_WSG_PIXEL_PUSH_SIDE_1,
    PA_WSG_PIXEL_PUSH_SIDE_2,
    PA_WSG_PIXEL_HURT,
    PA_WSG_PIXEL_WIN,
    PA_WSG_PIXEL_ICON,
    PA_WSG_GIRL_SOUTH,
    PA_WSG_GIRL_WALK_SOUTH,
    PA_WSG_GIRL_NORTH,
    PA_WSG_GIRL_WALK_NORTH,
    PA_WSG_GIRL_SIDE,
    PA_WSG_GIRL_WALK_SIDE_1,
    PA_WSG_GIRL_WALK_SIDE_2,
    PA_WSG_GIRL_PUSH_SOUTH_1,
    PA_WSG_GIRL_PUSH_SOUTH_2,
    PA_WSG_GIRL_PUSH_NORTH_1,
    PA_WSG_GIRL_PUSH_NORTH_2,
    PA_WSG_GIRL_PUSH_SIDE_1,
    PA_WSG_GIRL_PUSH_SIDE_2,
    PA_WSG_GIRL_HURT,
    PA_WSG_GIRL_WIN,
    PA_WSG_GIRL_ICON,
    // PA_WSG_BLOCK,
    // PA_WSG_BONUS_BLOCK,
    PA_WSG_ENEMY_SOUTH,
    PA_WSG_ENEMY_NORTH,
    PA_WSG_ENEMY_SIDE_1,
    PA_WSG_ENEMY_SIDE_2,
    PA_WSG_ENEMY_DRILL_SOUTH,
    PA_WSG_ENEMY_DRILL_NORTH,
    PA_WSG_ENEMY_DRILL_SIDE_1,
    PA_WSG_ENEMY_DRILL_SIDE_2,
    PA_WSG_ENEMY_STUN,
    PA_WSG_BREAK_BLOCK,
    PA_WSG_BREAK_BLOCK_1,
    PA_WSG_BREAK_BLOCK_2,
    PA_WSG_BREAK_BLOCK_3,
    PA_WSG_BLOCK_FRAGMENT,
    PA_WSG_WALL_0,
    PA_WSG_WALL_1,
    PA_WSG_WALL_2,
    PA_WSG_WALL_3,
    PA_WSG_WALL_4,
    PA_WSG_WALL_5,
    PA_WSG_WALL_6,
    PA_WSG_WALL_7,
    PA_WSG_BLOCK_BLUE,
    PA_WSG_BLOCK_RED,
    PA_WSG_BLOCK_YELLOW,
    PA_WSG_BLOCK_MAGENTA,
    PA_WSG_BONUS_BLOCK_0,
    PA_WSG_BONUS_BLOCK_1,
    PA_WSG_BONUS_BLOCK_2
} paWsgIndex_t;

#endif