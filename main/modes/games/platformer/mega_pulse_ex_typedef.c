#include "mega_pulse_ex_typedef.h"

//==============================================================================
// Constant Variables
//==============================================================================

// Note, must match order and size of mg_bgmEnum_t
const cnfsFileIdx_t MG_BGMS[] = {BGM_DEADEYE_CHIRPZI_MID,    BGM_BOSS_DEADEYE_CHIRPZI_MID, BGM_DRAIN_BAT_MID,
                                 BGM_BOSS_DRAIN_BAT_MID,     BGM_FLARE_GRYFFYN_MID,        BGM_BOSS_FLARE_GRIFFIN_MID,
                                 BGM_GRIND_PANGOLIN_MID,     BGM_BOSS_GRIND_PANGOLIN_MID,  BGM_KINETIC_DONUT_MID,
                                 BGM_BOSS_KINETIC_DONUT_MID, BGM_RIP_BARONESS_MID,         BGM_BOSS_HANK_WADDLE_MID,
                                 BGM_SEVER_YAGATA_MID,       BGM_BOSS_SEVER_YAGATA_MID,    BGM_SMASH_GORILLA_MID,
                                 BGM_BOSS_SMASH_GORILLA_MID, BGM_BOSS_TRASH_MAN_MID,       BGM_BIGMA_MID,
                                 BGM_BOSS_BIGMA_MID,         BGM_LEVEL_CLEAR_JINGLE_MID,   BGM_POST_FIGHT_MID,
                                 BGM_PRE_FIGHT_MID,          BGM_INTRO_STAGE_MID,          BGM_STAGE_SELECT_MID,
                                 BGM_NAME_ENTRY_MID,         MAXIMUM_HYPE_CREDITS_MID,     BGM_OVO_LIVES_MID,};

/*
const int MG_1x2_TILE_COLLISION_OFFSETS_IN_PIXELS[]
    = {0, 8, MG_EDGE_BLR, 0, -8, MG_EDGE_TLR};

const int MG_TILE_COLLISION_OFFSETS_1x2_BOTTOM_EDGE[]
    = {-7, 15, 0, 15, 6, 15};

const int MG_TILE_COLLISION_OFFSETS_1x2_TOP_EDGE[]
    = {-7, -15, 0, -15, 7, -15};

const int MG_TILE_COLLISION_OFFSETS_1x2_RIGHT_EDGE[]
    = {8, 14, 8, 0, 8, -14};

const int MG_TILE_COLLISION_OFFSETS_1x2_LEFT_EDGE[]
    = {-7, 14, -7, 0, -7, -14};
*/

const char MG_cheatModeNVSKey[] = "mg_cheatMode";
const char MG_abilitiesNVSKey[] = "mg_abilities";

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_bottomEdge
    = {.collisionPoints = {{.x = -7, .y = 15}, {.x = 0, .y = 15}, {.x = 6, .y = 15}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_topEdge
    = {.collisionPoints = {{.x = -7, .y = -15}, {.x = 0, .y = -15}, {.x = 7, .y = -15}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_rightEdge
    = {.collisionPoints = {{.x = 8, .y = 14}, {.x = 8, .y = 0}, {.x = 8, .y = -14}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_leftEdge
    = {.collisionPoints = {{.x = -7, .y = 14}, {.x = -7, .y = 0}, {.x = -7, .y = -14}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_topEdge_dash_slide
    = {.collisionPoints = {{.x = -7, .y = 0}, {.x = 0, .y = 0}, {.x = 7, .y = -0}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_rightEdge_dash_slide
    = {.collisionPoints = {{.x = 8, .y = 14}, {.x = 8, .y = 0}, {.x = 8, .y = 1}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_leftEdge_dash_slide
    = {.collisionPoints = {{.x = -7, .y = 14}, {.x = -7, .y = 0}, {.x = -7, .y = 1}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_1x2 = {.bottomEdge = &mgTileCollisionOffsets_1x2_bottomEdge,
                                                        .topEdge    = &mgTileCollisionOffsets_1x2_topEdge,
                                                        .rightEdge  = &mgTileCollisionOffsets_1x2_rightEdge,
                                                        .leftEdge   = &mgTileCollisionOffsets_1x2_leftEdge};

const mg_EntityTileCollider_t entityTileCollider_1x2_dash_slide
    = {.bottomEdge = &mgTileCollisionOffsets_1x2_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_1x2_topEdge_dash_slide,
       .rightEdge  = &mgTileCollisionOffsets_1x2_rightEdge_dash_slide,
       .leftEdge   = &mgTileCollisionOffsets_1x2_leftEdge_dash_slide};

// sever yataga origin: 49, 35
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_sever_yataga_bottomEdge
    = {.collisionPoints = {{.x = -17, .y = 35}, {.x = 0, .y = 35}, {.x = 16, .y = 35}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_sever_yataga_topEdge
    = {.collisionPoints = {{.x = -17, .y = -19}, {.x = 0, .y = -19}, {.x = 16, .y = -19}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_sever_yataga_rightEdge
    = {.collisionPoints = {{.x = 16, .y = 35}, {.x = 16, .y = 0}, {.x = 16, .y = -19}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_sever_yataga_leftEdge
    = {.collisionPoints = {{.x = -17, .y = 35}, {.x = -17, .y = 0}, {.x = -17, .y = -19}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_sever_yataga
    = {.bottomEdge = &mgTileCollisionOffsets_sever_yataga_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_sever_yataga_topEdge,
       .rightEdge  = &mgTileCollisionOffsets_sever_yataga_rightEdge,
       .leftEdge   = &mgTileCollisionOffsets_sever_yataga_leftEdge};

// smash gorilla origin: 49, 35
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_smash_gorilla_bottomEdge
    = {.collisionPoints = {{.x = -15, .y = 34}, {.x = 0, .y = 34}, {.x = 14, .y = 34}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_smash_gorilla_topEdge
    = {.collisionPoints = {{.x = -15, .y = -25}, {.x = 0, .y = -25}, {.x = 14, .y = -25}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_smash_gorilla_rightEdge
    = {.collisionPoints = {{.x = 14, .y = 34}, {.x = 14, .y = 0}, {.x = 14, .y = -25}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_smash_gorilla_leftEdge
    = {.collisionPoints = {{.x = -15, .y = 34}, {.x = -15, .y = 0}, {.x = -15, .y = -25}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_smash_gorilla
    = {.bottomEdge = &mgTileCollisionOffsets_smash_gorilla_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_smash_gorilla_topEdge,
       .rightEdge  = &mgTileCollisionOffsets_smash_gorilla_rightEdge,
       .leftEdge   = &mgTileCollisionOffsets_smash_gorilla_leftEdge};

// grind pangolin origin: 33, 28
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_bottomEdge
    = {.collisionPoints = {{.x = -30, .y = 27}, {.x = 0, .y = 27}, {.x = 30, .y = 27}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_topEdge
    = {.collisionPoints = {{.x = -30, .y = -11}, {.x = 0, .y = -11}, {.x = 30, .y = -11}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_rightEdge
    = {.collisionPoints = {{.x = 30, .y = 25}, {.x = 30, .y = 0}, {.x = 30, .y = -11}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_leftEdge
    = {.collisionPoints = {{.x = -30, .y = 25}, {.x = -30, .y = 0}, {.x = -30, .y = -11}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_grind_pangolin
    = {.bottomEdge = &mgTileCollisionOffsets_grind_pangolin_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_grind_pangolin_topEdge,
       .rightEdge  = &mgTileCollisionOffsets_grind_pangolin_rightEdge,
       .leftEdge   = &mgTileCollisionOffsets_grind_pangolin_leftEdge};

// grind pangolin rolling origin: 19, 15
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_rolling_bottomEdge
    = {.collisionPoints = {{.x = -10, .y = 15}, {.x = 0, .y = 15}, {.x = 10, .y = 15}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_rolling_topEdge
    = {.collisionPoints = {{.x = -10, .y = -15}, {.x = 0, .y = -15}, {.x = 10, .y = -15}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_rolling_rightEdge
    = {.collisionPoints = {{.x = 10, .y = 14}, {.x = 10, .y = 0}, {.x = 10, .y = -15}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_rolling_leftEdge
    = {.collisionPoints = {{.x = -10, .y = 14}, {.x = -10, .y = 0}, {.x = -10, .y = -15}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_grind_pangolin_rolling
    = {.bottomEdge = &mgTileCollisionOffsets_grind_pangolin_rolling_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_grind_pangolin_rolling_topEdge,
       .rightEdge  = &mgTileCollisionOffsets_grind_pangolin_rolling_rightEdge,
       .leftEdge   = &mgTileCollisionOffsets_grind_pangolin_rolling_leftEdge};

// drain bat origin: 30, 31
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_drain_bat_bottomEdge
    = {.collisionPoints = {{.x = -17, .y = 31}, {.x = 0, .y = 31}, {.x = 17, .y = 31}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_drain_bat_topEdge
    = {.collisionPoints = {{.x = -17, .y = -15}, {.x = 0, .y = -15}, {.x = 17, .y = -15}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_drain_bat_rightEdge
    = {.collisionPoints = {{.x = 17, .y = 31}, {.x = 17, .y = 0}, {.x = 17, .y = -15}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_drain_bat_leftEdge
    = {.collisionPoints = {{.x = -17, .y = 31}, {.x = -17, .y = 0}, {.x = -17, .y = -15}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_drain_bat
    = {.bottomEdge = &mgTileCollisionOffsets_drain_bat_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_drain_bat_topEdge,
       .rightEdge  = &mgTileCollisionOffsets_drain_bat_rightEdge,
       .leftEdge   = &mgTileCollisionOffsets_drain_bat_leftEdge};

// kinetic donut origin: 21, 25
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_kinetic_donut_bottomEdge
    = {.collisionPoints = {{.x = -15, .y = 24}, {.x = 0, .y = 24}, {.x = 15, .y = 24}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_kinetic_donut_topEdge
    = {.collisionPoints = {{.x = -15, .y = -9}, {.x = 0, .y = -9}, {.x = 15, .y = -9}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_kinetic_donut_rightEdge
    = {.collisionPoints = {{.x = 16, .y = 23}, {.x = 16, .y = 0}, {.x = 16, .y = -9}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_kinetic_donut_leftEdge
    = {.collisionPoints = {{.x = -16, .y = 23}, {.x = -13, .y = 0}, {.x = -16, .y = -9}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_kinetic_donut
    = {.bottomEdge = &mgTileCollisionOffsets_kinetic_donut_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_kinetic_donut_topEdge,
       .rightEdge  = &mgTileCollisionOffsets_kinetic_donut_rightEdge,
       .leftEdge   = &mgTileCollisionOffsets_kinetic_donut_leftEdge};

// flare gryffyn origin: 40, 32
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_bottomEdge
    = {.collisionPoints = {{.x = -24, .y = 30}, {.x = 0, .y = 30}, {.x = 24, .y = 30}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_topEdge
    = {.collisionPoints = {{.x = -24, .y = -32}, {.x = 0, .y = -32}, {.x = 24, .y = -32}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_rightEdge
    = {.collisionPoints = {{.x = 24, .y = 30}, {.x = 24, .y = 0}, {.x = 24, .y = -32}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_leftEdge
    = {.collisionPoints = {{.x = -24, .y = 30}, {.x = -24, .y = 0}, {.x = -24, .y = -32}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_flare_gryffyn
    = {.bottomEdge = &mgTileCollisionOffsets_flare_gryffyn_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_flare_gryffyn_topEdge,
       .rightEdge  = &mgTileCollisionOffsets_flare_gryffyn_rightEdge,
       .leftEdge   = &mgTileCollisionOffsets_flare_gryffyn_leftEdge};

// flare gryffyn jumping origin: 48, 44
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_jumping_bottomEdge
    = {.collisionPoints = {{.x = -19, .y = 41}, {.x = 0, .y = 41}, {.x = 18, .y = 41}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_jumping_topEdge
    = {.collisionPoints = {{.x = -19, .y = -44}, {.x = 0, .y = -44}, {.x = 18, .y = -44}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_jumping_rightEdge
    = {.collisionPoints = {{.x = 18, .y = 41}, {.x = 18, .y = 0}, {.x = 18, .y = -44}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_jumping_leftEdge
    = {.collisionPoints = {{.x = -19, .y = 41}, {.x = -19, .y = 0}, {.x = -19, .y = -44}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_flare_gryffyn_jumping
    = {.bottomEdge = &mgTileCollisionOffsets_flare_gryffyn_jumping_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_flare_gryffyn_jumping_topEdge,
       .rightEdge  = &mgTileCollisionOffsets_flare_gryffyn_jumping_rightEdge,
       .leftEdge   = &mgTileCollisionOffsets_flare_gryffyn_jumping_leftEdge};

// deadeye chirpzi origin: 40, 34
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_deadeye_chirpzi_bottomEdge
    = {.collisionPoints = {{.x = -17, .y = 33}, {.x = 0, .y = 33}, {.x = 16, .y = 33}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_deadeye_chirpzi_topEdge
    = {.collisionPoints = {{.x = -17, .y = -29}, {.x = 0, .y = -29}, {.x = 16, .y = -29}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_deadeye_chirpzi_rightEdge
    = {.collisionPoints = {{.x = 16, .y = 33}, {.x = 16, .y = 0}, {.x = 16, .y = -29}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_deadeye_chirpzi_leftEdge
    = {.collisionPoints = {{.x = -17, .y = 33}, {.x = -17, .y = 0}, {.x = -17, .y = -29}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_deadeye_chirpzi
    = {.bottomEdge = &mgTileCollisionOffsets_deadeye_chirpzi_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_deadeye_chirpzi_topEdge,
       .rightEdge  = &mgTileCollisionOffsets_deadeye_chirpzi_rightEdge,
       .leftEdge   = &mgTileCollisionOffsets_deadeye_chirpzi_leftEdge};

// trash man origin: 38, 33
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_trash_man_bottomEdge
    = {.collisionPoints = {{.x = -30, .y = 32}, {.x = 0, .y = 32}, {.x = 29, .y = 32}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_trash_man_topEdge
    = {.collisionPoints = {{.x = -30, .y = -29}, {.x = 0, .y = -29}, {.x = 29, .y = -29}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_trash_man_rightEdge
    = {.collisionPoints = {{.x = 30, .y = 31}, {.x = 30, .y = 0}, {.x = 30, .y = -29}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_trash_man_leftEdge
    = {.collisionPoints = {{.x = -30, .y = 31}, {.x = -30, .y = 0}, {.x = -30, .y = -29}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_trash_man
    = {.bottomEdge = &mgTileCollisionOffsets_trash_man_bottomEdge,
       .topEdge    = &mgTileCollisionOffsets_trash_man_topEdge,
       .rightEdge  = &mgTileCollisionOffsets_trash_man_rightEdge,
       .leftEdge   = &mgTileCollisionOffsets_trash_man_leftEdge};

// bigma origin: 33, 44
const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_bigma_bottomEdge
    = {.collisionPoints = {{.x = -17, .y = 42}, {.x = 0, .y = 42}, {.x = 17, .y = 42}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_bigma_topEdge
    = {.collisionPoints = {{.x = -17, .y = -44}, {.x = 0, .y = -44}, {.x = 17, .y = -44}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_bigma_rightEdge
    = {.collisionPoints = {{.x = 17, .y = 42}, {.x = 17, .y = 0}, {.x = 17, .y = -44}}, .size = 3};

const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_bigma_leftEdge
    = {.collisionPoints = {{.x = -17, .y = 42}, {.x = -17, .y = 0}, {.x = -17, .y = -44}}, .size = 3};

const mg_EntityTileCollider_t entityTileCollider_bigma = {.bottomEdge = &mgTileCollisionOffsets_bigma_bottomEdge,
                                                          .topEdge    = &mgTileCollisionOffsets_bigma_topEdge,
                                                          .rightEdge  = &mgTileCollisionOffsets_bigma_rightEdge,
                                                          .leftEdge   = &mgTileCollisionOffsets_bigma_leftEdge};

// Note: none of these names match actual colors
const paletteColor_t bgGradientGray[]          = {c001, c112, c223, c334};
const paletteColor_t bgGradientPurple[]        = {c202, c203, c204, c205};
const paletteColor_t bgGradientBlue[]          = {c102, c103, c104, c105};
const paletteColor_t bgGradientCyan[]          = {c003, c013, c023, c033};
const paletteColor_t bgGradientGreen[]         = {c001, c111, c221, c331};
const paletteColor_t bgGradientYellow[]        = {c202, c312, c422, c532};
const paletteColor_t bgGradientOrange[]        = {c040, c230, c220, c210};
const paletteColor_t bgGradientRed[]           = {c101, c201, c301, c411};
const paletteColor_t bgGradientFinalShowdown[] = {c000, c000, c000, c000};
const paletteColor_t bgGradientMenu[]          = {c001, c012, c123, c234};

const mgLeveldef_t leveldef[] = {
    {.filename           = LEVEL_SELECT_RAW,
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_LEVEL_SELECT,
     .mainBgmIndex       = MG_BGM_STAGE_SELECT,
     .bossBgmIndex       = MG_BGM_NULL,
     .bgColors           = bgGradientMenu},
    {.filename           = DONUT_RAW,
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_KINETIC_DONUT,
     .mainBgmIndex       = MG_BGM_KINETIC_DONUT,
     .bossBgmIndex       = MG_BGM_BOSS_KINETIC_DONUT,
     .bgColors           = bgGradientBlue},
    {.filename           = GRIND_RAW,
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_GRIND_PANGOLIN,
     .mainBgmIndex       = MG_BGM_GRIND_PANGOLIN,
     .bossBgmIndex       = MG_BGM_BOSS_GRIND_PANGOLIN,
     .bgColors           = bgGradientPurple},
    {.filename           = SEVER_RAW,
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_SEVER_YATAGA,
     .mainBgmIndex       = MG_BGM_SEVER_YATAGA,
     .bossBgmIndex       = MG_BGM_BOSS_SEVER_YATAGA,
     .bgColors           = bgGradientCyan},
    {.filename           = DUMP_RAW,
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_TRASH_MAN,
     .mainBgmIndex       = MG_BGM_RIP_BARONESS,
     .bossBgmIndex       = MG_BGM_BOSS_TRASH_MAN,
     .bgColors           = bgGradientOrange},
    {.filename           = GAUNTLET_RAW,
     .timeLimit          = 240,
     .defaultWsgSetIndex = MG_WSGSET_BIGMA,
     .mainBgmIndex       = MG_BGM_KINETIC_DONUT,
     .bossBgmIndex       = MG_BGM_BOSS_BIGMA,
     .bgColors           = bgGradientMenu},
    {.filename           = SMASH_RAW,
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_SMASH_GORILLA,
     .mainBgmIndex       = MG_BGM_SMASH_GORILLA,
     .bossBgmIndex       = MG_BGM_BOSS_SMASH_GORILLA,
     .bgColors           = bgGradientYellow},
    {.filename           = DEADEYE_RAW,
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_DEADEYE_CHIRPZI,
     .mainBgmIndex       = MG_BGM_DEADEYE_CHIRPZI,
     .bossBgmIndex       = MG_BGM_BOSS_DEADEYE_CHIRPZI,
     .bgColors           = bgGradientGreen},
    {.filename           = DRAIN_RAW,
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_DRAIN_BAT,
     .mainBgmIndex       = MG_BGM_DRAIN_BAT,
     .bossBgmIndex       = MG_BGM_BOSS_DRAIN_BAT,
     .bgColors           = bgGradientGray},
    {.filename           = GRYFFYN_RAW,
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_FLARE_GRYFFYN,
     .mainBgmIndex       = MG_BGM_FLARE_GRYFFYN,
     .bossBgmIndex       = MG_BGM_BOSS_FLARE_GRYFFYN,
     .bgColors           = bgGradientRed},
    {.filename           = INTRO_RAW, // 10
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_BIGMA,
     .mainBgmIndex       = MG_BGM_INTRO_STAGE,
     .bossBgmIndex       = MG_BGM_BOSS_BIGMA,
     .bgColors           = bgGradientBlue},
    {.filename           = BOSS_TEST_DONUT_RAW, // 11 This is actually used for the boss rush. DO NOT DELETE!
     .timeLimit          = 500,
     .defaultWsgSetIndex = MG_WSGSET_KINETIC_DONUT,
     .mainBgmIndex       = MG_BGM_FINAL_MEGAJAM,
     .bossBgmIndex       = MG_BGM_BOSS_KINETIC_DONUT,
     .bgColors = bgGradientFinalShowdown}, // starts black to fit the script, then becomes colored with each boss.
    {.filename           = SHOWDOWN_RAW,   // Bigma2 & Hank fight (12)
     .timeLimit          = 200,
     .defaultWsgSetIndex = MG_WSGSET_HANK_WADDLE,
     .mainBgmIndex       = MG_BGM_PRE_FIGHT,
     .bossBgmIndex       = MG_BGM_BOSS_HANK_WADDLE,//use this for final showdown
     .bgColors           = bgGradientFinalShowdown},
    {.filename           = BOSS_TEST_ROOM_RAW,//13 It's just here for JVeg
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_KINETIC_DONUT,
     .mainBgmIndex       = MG_BGM_KINETIC_DONUT,
     .bossBgmIndex       = MG_BGM_BOSS_SEVER_YATAGA,
     .bgColors           = bgGradientPurple},
     {.filename          = BOSS_TEST_ROOM_RAW,//14, PLACEHOLDER just to have credits music and 'Ovo Lives'! (garbotnik home 2 from attic)
     .timeLimit          = 180,
     .defaultWsgSetIndex = MG_WSGSET_KINETIC_DONUT,
     .mainBgmIndex       = MG_BGM_MAXIMUM_HYPE_CREDITS,
     .bossBgmIndex       = MG_BGM_OVO_LIVES,
     .bgColors           = bgGradientPurple},
};
