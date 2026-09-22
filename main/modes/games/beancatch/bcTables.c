#include "bcTables.h"
#include <cnfs_image.h>

//==============================================================================
// Constant Variables
//==============================================================================

const cnfsFileIdx_t BC_WSGS[] = {
    //WSGs in order
    BC_BEAN_UL_00_WSG,
    BC_BEAN_UL_01_WSG,
    BC_BEAN_UL_02_WSG,
    BC_BEAN_UL_03_WSG,
    BC_BEAN_UL_04_WSG,
    BC_BEAN_DL_00_WSG,
    BC_BEAN_DL_01_WSG,
    BC_BEAN_DL_02_WSG,
    BC_BEAN_DL_03_WSG,
    BC_BEAN_DL_04_WSG,
    BC_BEAN_UR_00_WSG,
    BC_BEAN_UR_01_WSG,
    BC_BEAN_UR_02_WSG,
    BC_BEAN_UR_03_WSG,
    BC_BEAN_UR_04_WSG,
    BC_BEAN_UL_00_WSG,
    BC_BEAN_UL_01_WSG,
    BC_BEAN_UL_02_WSG,
    BC_BEAN_UL_03_WSG,
    BC_BEAN_UL_04_WSG,
    BC_BEAN_DROP_L_WSG,
    BC_BEAN_DROP_R_WSG,
    BC_PLAYER_UL_WSG,
    BC_PLAYER_DL_WSG,
    BC_PLAYER_UR_WSG,
    BC_PLAYER_DR_WSG,
    BC_BEANBERT_L_WSG,
    BC_BEANBERT_R_WSG,
    BC_SWEARING_WSG,
    BC_BUBBLE_PTR_L_WSG,
    BC_BUBBLE_PTR_R_WSG,
    BC_EGGY_DEVITO_WSG,
    BC_STRIKE_LBL_WSG,
    BC_STRIKE_ICON_WSG,
    BC_GAME_A_LBL_WSG,
    BC_GAME_B_LBL_WSG,
    BC_AM_LABEL_WSG,
    BC_ALARM_ICON_WSG,
    BC_ALARM_ANIM_1_WSG,
    BC_ALARM_ANIN_2_WSG,
    BC_HOURS_SEP_WSG,
    BC_BEAN_CATCH_BG_WSG,
    BC_BOTTOM_LBL_WSG,
    BC_TOP_LABEL_WSG
};

const bc_LcdSegment_t BC_LCD_SEGMENTS[] = {
    {.wsgIndex = BC_WSG_BEAN_UL_00,  .x =   8, .y =  73}, //BC_SEG_BEAN_UL_00
    {.wsgIndex = BC_WSG_BEAN_UL_01,  .x =  24, .y =  83}, //BC_SEG_BEAN_UL_01
    {.wsgIndex = BC_WSG_BEAN_UL_02,  .x =  39, .y =  93}, //BC_SEG_BEAN_UL_02
    {.wsgIndex = BC_WSG_BEAN_UL_03,  .x =  54, .y = 101}, //BC_SEG_BEAN_UL_03
    {.wsgIndex = BC_WSG_BEAN_UL_04,  .x =  69, .y = 112}, //BC_SEG_BEAN_UL_04
    {.wsgIndex = BC_WSG_BEAN_DL_00,  .x =   9, .y = 107}, //BC_SEG_BEAN_DL_00
    {.wsgIndex = BC_WSG_BEAN_DL_01,  .x =  24, .y = 116}, //BC_SEG_BEAN_DL_01
    {.wsgIndex = BC_WSG_BEAN_DL_02,  .x =  38, .y = 126}, //BC_SEG_BEAN_DL_02
    {.wsgIndex = BC_WSG_BEAN_DL_03,  .x =  55, .y = 135}, //BC_SEG_BEAN_DL_03
    {.wsgIndex = BC_WSG_BEAN_DL_04,  .x =  71, .y = 146}, //BC_SEG_BEAN_DL_04
    {.wsgIndex = BC_WSG_BEAN_UR_00,  .x = 264, .y =  74}, //BC_SEG_BEAN_UR_00
    {.wsgIndex = BC_WSG_BEAN_UR_01,  .x = 246, .y =  83}, //BC_SEG_BEAN_UR_01
    {.wsgIndex = BC_WSG_BEAN_UR_02,  .x = 232, .y =  93}, //BC_SEG_BEAN_UR_02
    {.wsgIndex = BC_WSG_BEAN_UR_03,  .x = 216, .y = 102}, //BC_SEG_BEAN_UR_03
    {.wsgIndex = BC_WSG_BEAN_UR_04,  .x = 199, .y = 113}, //BC_SEG_BEAN_UR_04
    {.wsgIndex = BC_WSG_BEAN_DR_00,  .x = 261, .y = 106}, //BC_SEG_BEAN_DR_00
    {.wsgIndex = BC_WSG_BEAN_DR_01,  .x = 244, .y = 116}, //BC_SEG_BEAN_DR_01
    {.wsgIndex = BC_WSG_BEAN_DR_02,  .x = 229, .y = 126}, //BC_SEG_BEAN_DR_02
    {.wsgIndex = BC_WSG_BEAN_DR_03,  .x = 214, .y = 134}, //BC_SEG_BEAN_DR_03
    {.wsgIndex = BC_WSG_BEAN_DR_04,  .x = 197, .y = 145}, //BC_SEG_BEAN_DR_04
    {.wsgIndex = BC_WSG_BEAN_DROP_L, .x =  73, .y = 178}, //BC_SEG_BEAN_DROP_L
    {.wsgIndex = BC_WSG_BEAN_DROP_R, .x = 178, .y = 178}, //BC_SEG_BEAN_DROP_R
    {.wsgIndex = BC_WSG_PLAYER_UL,   .x =  71, .y = 123}, //BC_SEG_PLAYER_UL
    {.wsgIndex = BC_WSG_PLAYER_DL,   .x =  76, .y = 151}, //BC_SEG_PLAYER_DL
    {.wsgIndex = BC_WSG_PLAYER_UR,   .x = 181, .y = 123}, //BC_SEG_PLAYER_UR
    {.wsgIndex = BC_WSG_PLAYER_DR,   .x = 180, .y = 151}, //BC_SEG_PLAYER_DR
    {.wsgIndex = BC_WSG_BEANBERT_L,  .x = 100, .y = 107}, //BC_SEG_BEANBERT_L
    {.wsgIndex = BC_WSG_BEANBERT_R,  .x = 136, .y = 107}, //BC_SEG_BEANBERT_R
    {.wsgIndex = BC_WSG_SWEARING,    .x = 103, .y =  73}, //BC_SEG_SWEARING
    {.wsgIndex = BC_WSG_BUBBLE_PTR_L,.x = 124, .y =  99}, //BC_SEG_BUBBLE_PTR_L
    {.wsgIndex = BC_WSG_BUBBLE_PTR_R,.x = 142, .y =  99}, //BC_SEG_BUBBLE_PTR_R
    {.wsgIndex = BC_WSG_EGGY_DEVITO, .x = 178, .y =  52}, //BC_SEG_EGGY_DEVITO
    {.wsgIndex = BC_WSG_STRIKE_LBL,  .x = 128, .y = 178}, //BC_SEG_STRIKE_LBL
    {.wsgIndex = BC_WSG_STRIKE_ICON, .x = 119, .y = 188}, //BC_SEG_STRIKE_ICON_00
    {.wsgIndex = BC_WSG_STRIKE_ICON, .x = 137, .y = 188}, //BC_SEG_STRIKE_ICON_01
    {.wsgIndex = BC_WSG_STRIKE_ICON, .x = 155, .y = 188}, //BC_SEG_STRIKE_ICON_02
    {.wsgIndex = BC_WSG_GAME_A_LBL,  .x =  27, .y = 187}, //BC_SEG_GAME_A_LBL
    {.wsgIndex = BC_WSG_GAME_B_LBL,  .x = 217, .y = 187}, //BC_SEG_GAME_B_LBL
    {.wsgIndex = BC_WSG_AM_LABEL,    .x =  21, .y =  53}, //BC_SEG_AM_LABEL
    {.wsgIndex = BC_WSG_ALARM_ICON,  .x = 126, .y =  51}, //BC_SEG_ALARM_ICON
    {.wsgIndex = BC_WSG_ALARM_ANIM_1,.x = 128, .y =  46}, //BC_SEG_ALARM_ANIM_1
    {.wsgIndex = BC_WSG_ALARM_ANIN_2,.x = 127, .y =  39}, //BC_SEG_ALARM_ANIM_2
    {.wsgIndex = BC_WSG_HOURS_SEP,   .x = 67,  .y =  56}  //BC_SEG_HOURS_SEP
};

const cnfsFileIdx_t BC_SOUND_MAP[] = {
    BC_BEEP_0_MID, 
    BC_BEEP_1_MID, 
    BC_BEEP_2_MID,
    BC_BEEP_3_MID,
    BC_SCORE_POINT_MID,
    BC_STRIKE_MID
};

const uint16_t BC_DIFFICULTY_MAX_BEANS[] = {
/*  0  1  2  3  4  5  6  7  9 10 11 12 13 14 15 16 MAX */
    2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 7, 7, 7, 9, 9, 9, 12
};
