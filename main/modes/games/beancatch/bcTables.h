#ifndef BEANCATCH_TABLES_INCLUDED
#define BEANCATCH_TABLES_INCLUDED

//==============================================================================
// Constants
//==============================================================================

//==============================================================================
// Macros
//==============================================================================

//==============================================================================
// Typedefs
//==============================================================================

//==============================================================================
// Structs
//==============================================================================

// typedef struct
// {
//     int32_t size;
//     vec_t collisionPoints[];
// } mg_EntityTileCollisionPointList_t;

// typedef struct
// {
//     const mg_EntityTileCollisionPointList_t* bottomEdge;
//     const mg_EntityTileCollisionPointList_t* topEdge;
//     const mg_EntityTileCollisionPointList_t* rightEdge;
//     const mg_EntityTileCollisionPointList_t* leftEdge;
// } mg_EntityTileCollider_t;

typedef struct
{
    bc_wsgIndex_t wsgIndex,
    uint32_t x,
    uint32_t y
} bc_LcdSegment_t;

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    BC_ST_NULL,
    BC_ST_CLOCK,
    BC_ST_GAME,
    BC_ST_GAME_OVER,
    BC_ST_SHOW_HISCORE
} bc_gameStateEnum_t;

typedef enum
{
    BC_WSG_BEAN_UL_00,
    BC_WSG_BEAN_UL_01,
    BC_WSG_BEAN_UL_02,
    BC_WSG_BEAN_UL_03,
    BC_WSG_BEAN_UL_04,
    BC_WSG_BEAN_DL_00,
    BC_WSG_BEAN_DL_01,
    BC_WSG_BEAN_DL_02,
    BC_WSG_BEAN_DL_03,
    BC_WSG_BEAN_DL_04,
    BC_WSG_BEAN_UR_00,
    BC_WSG_BEAN_UR_01,
    BC_WSG_BEAN_UR_02,
    BC_WSG_BEAN_UR_03,
    BC_WSG_BEAN_UR_04,
    BC_WSG_BEAN_DR_00,
    BC_WSG_BEAN_DR_01,
    BC_WSG_BEAN_DR_02,
    BC_WSG_BEAN_DR_03,
    BC_WSG_BEAN_DR_04,
    BC_WSG_BEAN_DROP_L,
    BC_WSG_BEAN_DROP_R,
    BC_WSG_PLAYER_UL,
    BC_WSG_PLAYER_DL,
    BC_WSG_PLAYER_UR,
    BC_WSG_PLAYER_DR,
    BC_WSG_BEANBERT_L,
    BC_WSG_BEANBERT_R,
    BC_WSG_SWEARING,
    BC_WSG_BUBBLE_PTR_L,
    BC_WSG_BUBBLE_PTR_R,
    BC_WSG_EGGY_DEVITO,
    BC_WSG_STRIKE_LBL,
    BC_WSG_STRIKE_ICON,
    BC_WSG_GAME_A_LBL,
    BC_WSG_GAME_B_LBL,
    BC_WSG_AM_LABEL,
    BC_WSG_ALARM_ICON,
    BC_WSG_ALARM_ANIM_1,
    BC_WSG_ALARM_ANIN_2,
    BC_WSG_HOURS_SEP,
    BC_WSG_BEAN_CATCH_BG,
    BC_WSG_BOTTOM_LBL,
    BC_WSG_TOP_LABEL
} bc_wsgIndex_t;

typedef enum
{
    BC_SEG_BEAN_UL_00,
    BC_SEG_BEAN_UL_01,
    BC_SEG_BEAN_UL_02,
    BC_SEG_BEAN_UL_03,
    BC_SEG_BEAN_UL_04,
    BC_SEG_BEAN_DL_00,
    BC_SEG_BEAN_DL_01,
    BC_SEG_BEAN_DL_02,
    BC_SEG_BEAN_DL_03,
    BC_SEG_BEAN_DL_04,
    BC_SEG_BEAN_UR_00,
    BC_SEG_BEAN_UR_01,
    BC_SEG_BEAN_UR_02,
    BC_SEG_BEAN_UR_03,
    BC_SEG_BEAN_UR_04,
    BC_SEG_BEAN_DR_00,
    BC_SEG_BEAN_DR_01,
    BC_SEG_BEAN_DR_02,
    BC_SEG_BEAN_DR_03,
    BC_SEG_BEAN_DR_04,
    BC_SEG_BEAN_DROP_L,
    BC_SEG_BEAN_DROP_R,
    BC_SEG_PLAYER_UL,
    BC_SEG_PLAYER_DL,
    BC_SEG_PLAYER_UR,
    BC_SEG_PLAYER_DR,
    BC_SEG_BEANBERT_L,
    BC_SEG_BEANBERT_R,
    BC_SEG_SWEARING,
    BC_SEG_BUBBLE_PTR_L,
    BC_SEG_BUBBLE_PTR_R,
    BC_SEG_EGGY_DEVITO,
    BC_SEG_STRIKE_LBL,
    BC_SEG_STRIKE_ICON_00,
    BC_SEG_STRIKE_ICON_01,
    BC_SEG_STRIKE_ICON_02,
    BC_SEG_GAME_A_LBL,
    BC_SEG_GAME_B_LBL,
    BC_SEG_AM_LABEL,
    BC_SEG_ALARM_ICON,
    BC_SEG_ALARM_ANIM_1,
    BC_SEG_ALARM_ANIN_2,
    BC_SEG_HOURS_SEP,
    BC_SEG_BEAN_CATCH_BG,
    BC_SEG_BOTTOM_LBL,
    BC_SEG_TOP_LABEL
} bc_segmentIndex_t;

//==============================================================================
// Extern Variables
//==============================================================================

// extern const cnfsFileIdx_t MG_BGMS[];
// extern const int MG_1x2_TILE_COLLISION_OFFSETS_IN_PIXELS[];
// extern const int MG_TILE_COLLISION_OFFSETS_1x2_BOTTOM_EDGE[];
// extern const int MG_TILE_COLLISION_OFFSETS_1x2_TOP_EDGE[];
// extern const int MG_TILE_COLLISION_OFFSETS_1x2_RIGHT_EDGE[];
// extern const int MG_TILE_COLLISION_OFFSETS_1x2_LEFT_EDGE[];
// extern const char MG_cheatModeNVSKey[];
// extern const char MG_abilitiesNVSKey[];
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_leftEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_topEdge_dash_slide;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_rightEdge_dash_slide;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_1x2_leftEdge_dash_slide;
// extern const mg_EntityTileCollider_t entityTileCollider_1x2;
// extern const mg_EntityTileCollider_t entityTileCollider_1x2_dash_slide;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_sever_yataga_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_sever_yataga_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_sever_yataga_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_sever_yataga_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_sever_yataga;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_smash_gorilla_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_smash_gorilla_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_smash_gorilla_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_smash_gorilla_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_smash_gorilla;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_grind_pangolin;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_rolling_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_rolling_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_rolling_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_grind_pangolin_rolling_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_grind_pangolin_rolling;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_drain_bat_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_drain_bat_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_drain_bat_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_drain_bat_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_drain_bat;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_kinetic_donut_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_kinetic_donut_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_kinetic_donut_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_kinetic_donut_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_kinetic_donut;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_flare_gryffyn;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_jumping_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_jumping_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_jumping_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_flare_gryffyn_jumping_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_flare_gryffyn_jumping;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_deadeye_chirpzi_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_deadeye_chirpzi_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_deadeye_chirpzi_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_deadeye_chirpzi_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_deadeye_chirpzi;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_trash_man_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_trash_man_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_trash_man_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_trash_man_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_trash_man;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_bigma_bottomEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_bigma_topEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_bigma_rightEdge;
// extern const mg_EntityTileCollisionPointList_t mgTileCollisionOffsets_bigma_leftEdge;
// extern const mg_EntityTileCollider_t entityTileCollider_bigma;
// extern const paletteColor_t bgGradientGray[];
// extern const paletteColor_t bgGradientPurple[];
// extern const paletteColor_t bgGradientBlue[];
// extern const paletteColor_t bgGradientCyan[];
// extern const paletteColor_t bgGradientGreen[];
// extern const paletteColor_t bgGradientYellow[];
// extern const paletteColor_t bgGradientOrange[];
// extern const paletteColor_t bgGradientRed[];
// extern const paletteColor_t bgGradientMenu[];
// extern const mgLeveldef_t leveldef[];

// platformerTrophies array is defined in megaPulseEx.c to avoid multiple definitions
// extern const trophyData_t platformerTrophies[];

//==============================================================================
// Function Declarations
//==============================================================================


#endif