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

#endif