//==============================================================================
// Includes
//==============================================================================

#include "mgSprite.h"

//==============================================================================
// Constants
//==============================================================================

const vec_t origin_8_8 = {.x = 8, .y = 8};

const vec_t origin_15_15 = {.x = 15, .y = 15};

const vec_t origin_11_11 = {.x = 11, .y = 11};

const vec_t origin_7_31 = {.x = 7, .y = 31};

const vec_t origin_31_7 = {.x = 31, .y = 7};

const vec_t origin_15_9 = {.x = 15, .y = 3};

const vec_t origin_15_11 = {.x = 15, .y = 11};

const vec_t origin_26_16 = {.x = 26, .y = 16};

const box_t box_16_16 = {.x0 = 0, .x1 = 15, .y0 = 0, .y1 = 15};

const box_t box_24_24 = {.x0 = 0, .x1 = 23, .y0 = 0, .y1 = 23};

const box_t box_16_32 = {.x0 = 7, .x1 = 22, .y0 = 0, .y1 = 30};

const box_t box_32_32 = {.x0 = 0, .x1 = 31, .y0 = 0, .y1 = 31};

const box_t box_16_64 = {.x0 = 0, .x1 = 15, .y0 = 0, .y1 = 63};

const box_t box_64_16 = {.x0 = 0, .x1 = 63, .y0 = 0, .y1 = 15};

const box_t box_30_30 = {.x0 = 0, .x1 = 29, .y0 = 0, .y1 = 29};

const box_t box_32_20 = {.x0 = 0, .x1 = 30, .y0 = 4, .y1 = 19};

const box_t box_charge_shot_lvl1 = {.x0 = 3, .x1 = 27, .y0 = 6, .y1 = 16};

const box_t box_charge_shot_max = {.x0 = 0, .x1 = 7, .y0 = 6, .y1 = 25};

const vec_t origin_sever_yataga = {.x = 49, .y = 35};

const box_t box_sever_yataga = {.x0 = 32, .x1 = 63, .y0 = 16, .y1 = 70};

const vec_t origin_smash_gorilla = {.x = 35, .y = 35};

const box_t box_smash_gorilla = {.x0 = 20, .x1 = 49, .y0 = 10, .y1 = 69};

const vec_t origin_grind_pangolin_standing = {.x = 33, .y = 28};
const box_t box_grind_pangolin_standing    = {.x0 = 3, .x1 = 63, .y0 = 17, .y1 = 55};
const vec_t origin_grind_pangolin_curled   = {.x = 22, .y = 19};
const box_t box_grind_pangolin_curled      = {.x0 = 7, .x1 = 36, .y0 = 9, .y1 = 38};
const vec_t origin_grind_pangolin_rolling  = {.x = 19, .y = 15};
const box_t box_grind_pangolin_rolling     = {.x0 = 9, .x1 = 29, .y0 = 0, .y1 = 30};
const vec_t origin_grind_pangolin_tail     = {.x = 37, .y = 17};
const box_t box_grind_pangolin_tail        = {.x0 = 3, .x1 = 71, .y0 = 16, .y1 = 31};

const vec_t origin_drain_bat = {.x = 30, .y = 31};
const box_t box_drain_bat    = {.x0 = 13, .x1 = 47, .y0 = 16, .y1 = 47};

const vec_t origin_kinetic_donut         = {.x = 21, .y = 25};
const box_t box_kinetic_donut            = {.x0 = 5, .x1 = 37, .y0 = 16, .y1 = 45};
const vec_t origin_kinetic_donut_dashing = {.x = 27, .y = 17};
const box_t box_kinetic_donut_dashing    = {.x0 = 4, .x1 = 54, .y0 = 7, .y1 = 42};

const vec_t origin_flare_gryffyn      = {.x = 40, .y = 32};
const box_t box_flare_gryffyn         = {.x0 = 16, .x1 = 69, .y0 = 0, .y1 = 62};
const vec_t origin_flare_gryffyn_jump = {.x = 48, .y = 44};
const box_t box_flare_gryffyn_jump    = {.x0 = 29, .x1 = 66, .y0 = 0, .y1 = 85};

const vec_t origin_deadeye_chirpzi       = {.x = 40, .y = 34};
const box_t box_deadeye_chirpzi          = {.x0 = 23, .x1 = 56, .y0 = 11, .y1 = 67};
const vec_t origin_deadeye_chirpzi_screw = {.x = 23, .y = 23};
const box_t box_deadeye_chirpzi_screw    = {.x0 = 7, .x1 = 40, .y0 = 7, .y1 = 39};

const vec_t origin_trash_man = {.x = 38, .y = 33};
const box_t box_trash_man    = {.x0 = 8, .x1 = 68, .y0 = 9, .y1 = 65};

const vec_t origin_bigma          = {.x = 33, .y = 44};
const box_t box_bigma             = {.x0 = 16, .x1 = 50, .y0 = 0, .y1 = 86};
const vec_t origin_bigma_punching = {.x = 55, .y = 44};
const box_t box_bigma_punching    = {.x0 = 16, .x1 = 95, .y0 = 9, .y1 = 86};
const vec_t origin_bigma_jump     = {.x = 58, .y = 49};
const box_t box_bigma_jump1       = {.x0 = 32, .x1 = 84, .y0 = 20, .y1 = 97};
const box_t box_bigma_jump2       = {.x0 = 32, .x1 = 84, .y0 = 0, .y1 = 97};
