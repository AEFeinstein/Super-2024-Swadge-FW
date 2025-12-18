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

//==============================================================================
// Constants
//==============================================================================

extern const vec_t origin_8_8;

extern const vec_t origin_15_15;

extern const vec_t origin_11_11;

extern const vec_t origin_7_31;

extern const vec_t origin_31_7;

extern const vec_t origin_15_9;

extern const vec_t origin_15_11;

extern const vec_t origin_16_16;

extern const vec_t origin_26_16;

extern const box_t box_16_16;

extern const box_t box_24_24;

extern const box_t box_16_32;

extern const box_t box_32_32;

extern const box_t box_16_64;

extern const box_t box_64_16;

extern const box_t box_30_30;

extern const box_t box_32_20;

extern const box_t box_charge_shot_lvl1;

extern const box_t box_charge_shot_max;

extern const vec_t origin_sever_yataga;

extern const box_t box_sever_yataga;

extern const vec_t origin_smash_gorilla;

extern const box_t box_smash_gorilla;

extern const vec_t origin_grind_pangolin_standing;
extern const box_t box_grind_pangolin_standing;
extern const vec_t origin_grind_pangolin_curled;
extern const box_t box_grind_pangolin_curled;
extern const vec_t origin_grind_pangolin_rolling;
extern const box_t box_grind_pangolin_rolling;
extern const vec_t origin_grind_pangolin_tail;
extern const box_t box_grind_pangolin_tail;

extern const vec_t origin_drain_bat;
extern const box_t box_drain_bat;

extern const vec_t origin_kinetic_donut;
extern const box_t box_kinetic_donut;
extern const vec_t origin_kinetic_donut_dashing;
extern const box_t box_kinetic_donut_dashing;

extern const vec_t origin_flare_gryffyn;
extern const box_t box_flare_gryffyn;
extern const vec_t origin_flare_gryffyn_jump;
extern const box_t box_flare_gryffyn_jump;

extern const vec_t origin_deadeye_chirpzi;
extern const box_t box_deadeye_chirpzi;
extern const vec_t origin_deadeye_chirpzi_screw;
extern const box_t box_deadeye_chirpzi_screw;

extern const vec_t origin_trash_man;
extern const box_t box_trash_man;

extern const vec_t origin_bigma;
extern const box_t box_bigma;
extern const vec_t origin_bigma_punching;
extern const box_t box_bigma_punching;
extern const vec_t origin_bigma_jump;
extern const box_t box_bigma_jump1;
extern const box_t box_bigma_jump2;

extern const vec_t origin_hank;
extern const box_t box_hank;

#endif