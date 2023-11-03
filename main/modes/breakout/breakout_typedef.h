#ifndef BREAKOUT_TYPEDEF_INCLUDED
#define BREAKOUT_TYPEDEF_INCLUDED

typedef struct breakout_t breakout_t;
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
    SP_PADDLE_VERTICAL_0,
    SP_PADDLE_VERTICAL_1,
    SP_PADDLE_VERTICAL_2,
    SP_BALL,
    SP_BALL_0,
    SP_BALL_1,
    SP_BALL_2,
    SP_BOMB_0,
    SP_BOMB_1,
    SP_BOMB_2,
    SP_EXPLOSION_0,
    SP_EXPLOSION_1,
    SP_EXPLOSION_2,
    SP_EXPLOSION_3,
    SP_BALL_TRAIL_0,
    SP_BALL_TRAIL_1,
    SP_BALL_TRAIL_2,
    SP_BALL_TRAIL_3,
    SP_CHO_WALK_0,
    SP_CHO_WALK_1,
    SP_CHO_WALK_2,
    SP_CHO_WIN_0,
    SP_CHO_WIN_1,
    SP_CRAWLER
} spriteDef_t;

#endif