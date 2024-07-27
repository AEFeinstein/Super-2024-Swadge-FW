#ifndef PLATFORMER_COMMON_TYPEDEF_INCLUDED
#define PLATFORMER_COMMON_TYPEDEF_INCLUDED

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
    PA_SP_PLAYER_JUMP,
    PA_SP_PLAYER_FALL,
    PA_SP_PLAYER_HURT,
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
    PA_SP_BLOCK_FRAGMENT,
    SP_WASP_1,
    SP_WASP_2,
    SP_WASP_DIVE,
    SP_1UP_1,
    SP_1UP_2,
    SP_1UP_3,
    SP_WAVEBALL_1,
    SP_WAVEBALL_2,
    SP_WAVEBALL_3,
    SP_ENEMY_BUSH_L2,
    SP_ENEMY_BUSH_L3,
    SP_DUSTBUNNY_L2_IDLE,
    SP_DUSTBUNNY_L2_CHARGE,
    SP_DUSTBUNNY_L2_JUMP,
    SP_DUSTBUNNY_L3_IDLE,
    SP_DUSTBUNNY_L3_CHARGE,
    SP_DUSTBUNNY_L3_JUMP,
    SP_WASP_L2_1,
    SP_WASP_L2_2,
    SP_WASP_L2_DIVE,
    SP_WASP_L3_1,
    SP_WASP_L3_2,
    SP_WASP_L3_DIVE,
    SP_CHECKPOINT_INACTIVE,
    SP_CHECKPOINT_ACTIVE_1,
    SP_CHECKPOINT_ACTIVE_2,
    SP_BOUNCE_BLOCK
} pa_spriteDef_t;

#endif