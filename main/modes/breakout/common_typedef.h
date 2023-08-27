#ifndef PLATFORMER_COMMON_TYPEDEF_INCLUDED
#define PLATFORMER_COMMON_TYPEDEF_INCLUDED

typedef struct platformer_t platformer_t;
typedef struct entityManager_t entityManager_t;
typedef struct tilemap_t tilemap_t;
typedef struct entity_t entity_t;

typedef enum {
    ST_NULL,
    ST_TITLE_SCREEN,
    ST_READY_SCREEN,
    ST_GAME,
    ST_DEAD,
    ST_LEVEL_CLEAR,
    ST_WORLD_CLEAR,
    ST_GAME_CLEAR,
    ST_GAME_OVER,
    ST_HIGH_SCORE_ENTRY,
    ST_HIGH_SCORE_TABLE,
    ST_PAUSE
} gameStateEnum_t;

typedef enum {
    BGM_NO_CHANGE,
    BGM_MAIN,
    BGM_ATHLETIC,
    BGM_UNDERGROUND,
    BGM_FORTRESS,
    BGM_NULL
} bgmEnum_t;

typedef enum {
    SP_PADDLE_0,
    SP_PADDLE_1,
    SP_PADDLE_2,
    SP_BALL,
    SP_BOMB_0,
    SP_BOMB_1,
    SP_BOMB_2,
    SP_EXPLOSION_0,
    SP_EXPLOSION_1,
    SP_EXPLOSION_2,
    SP_EXPLOSION_3,
    SP_HITBLOCK_BRICKS,
    SP_DUSTBUNNY_IDLE,
    SP_DUSTBUNNY_CHARGE,
    SP_DUSTBUNNY_JUMP,
    SP_GAMING_1,
    SP_GAMING_2,
    SP_GAMING_3,
    SP_MUSIC_1,
    SP_MUSIC_2,
    SP_MUSIC_3,
    SP_WARP_1,
    SP_WARP_2,
    SP_WARP_3,
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
} spriteDef_t;

#endif