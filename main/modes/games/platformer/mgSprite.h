#ifndef _MG_SPRITE_H_
#define _MG_SPRITE_H_

//==============================================================================
// Includes
//==============================================================================
#include <stdint.h>
#include "wsg.h"
#include "aabb_utils.h"
#include "vector2d.h"

//==============================================================================
// Structs
//==============================================================================
/*typedef struct
{
    int16_t x;
    int16_t y;
} mgSpriteOrigin_t;*/

typedef struct
{
    wsg_t* wsg;
    const vec_t* origin;
    const box_t* hitBox;
} mgSprite_t;

///==============================================================================
// Constants
//==============================================================================
static const vec_t origin_8_8 = {.x = 8, .y = 8};

static const vec_t origin_15_15 = {.x = 15, .y = 15};

static const vec_t origin_11_11 = {.x = 11, .y = 11};

static const vec_t origin_7_31 = {.x = 7, .y = 31};

static const vec_t origin_31_7 = {.x = 31, .y = 7};

static const vec_t origin_15_9 = {.x = 15, .y = 3};

static const vec_t origin_15_11 = {.x = 15, .y = 11};

static const vec_t origin_26_16 = {.x = 26, .y = 16};

static const box_t box_16_16 = {.x0 = 0, .x1 = 15, .y0 = 0, .y1 = 15};

static const box_t box_24_24 = {.x0 = 0, .x1 = 23, .y0 = 0, .y1 = 23};

static const box_t box_16_32 = {.x0 = 7, .x1 = 22, .y0 = 0, .y1 = 30};

static const box_t box_32_32 = {.x0 = 0, .x1 = 31, .y0 = 0, .y1 = 31};

static const box_t box_16_64 = {.x0 = 0, .x1 = 15, .y0 = 0, .y1 = 63};

static const box_t box_64_16 = {.x0 = 0, .x1 = 63, .y0 = 0, .y1 = 15};

static const box_t box_30_30 = {.x0 = 0, .x1 = 29, .y0 = 0, .y1 = 29};

static const box_t box_32_20 = {.x0 = 0, .x1 = 30, .y0 = 4, .y1 = 19};

static const box_t box_charge_shot_lvl1 = {.x0 = 3, .x1 = 27, .y0 = 6, .y1 = 16};

static const box_t box_charge_shot_max = {.x0 = 0, .x1 = 7, .y0 = 6, .y1 = 25};

static const vec_t origin_sever_yataga = {.x = 49, .y = 35};

static const box_t box_sever_yataga = {.x0 = 32, .x1 = 63, .y0 = 16, .y1 = 70};

static const vec_t origin_smash_gorilla = {.x = 35, .y = 35};

static const box_t box_smash_gorilla = {.x0 = 20, .x1 = 49, .y0 = 10, .y1 = 69};

static const vec_t origin_grind_pangolin_standing = {.x = 33, .y = 28};
static const box_t box_grind_pangolin_standing = {.x0 = 3, .x1 = 63, .y0 = 17, .y1 = 55};
static const vec_t origin_grind_pangolin_curled = {.x = 22, .y = 19};
static const box_t box_grind_pangolin_curled = {.x0 = 7, .x1 = 36, .y0 = 9, .y1 = 38};
static const vec_t origin_grind_pangolin_rolling = {.x = 19, .y = 15};
static const box_t box_grind_pangolin_rolling = {.x0 = 9, .x1 = 29, .y0 = 0, .y1 = 30};
static const vec_t origin_grind_pangolin_tail = {.x = 36, .y = 22};
static const box_t box_grind_pangolin_tail = {.x0 = 3, .x1 = 71, .y0 = 16, .y1 = 31};

static const vec_t origin_drain_bat = {.x = 30, .y = 31};
static const box_t box_drain_bat = {.x0 = 16, .x1 = 47, .y0 = 16, .y1 = 62};

static const vec_t origin_kinetic_donut = {.x = 21, .y = 25};
static const box_t box_kinetic_donut = {.x0 = 5, .x1 = 37, .y0 = 16, .y1 = 45};
static const vec_t origin_kinetic_donut_dashing = {.x = 27, .y = 21};
static const box_t box_kinetic_donut_dashing = {.x0 = 4, .x1 = 54, .y0 = 7, .y1 = 42};


#endif