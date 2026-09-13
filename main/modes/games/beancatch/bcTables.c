#include "bcTables.h"

//==============================================================================
// Constant Variables
//==============================================================================

const bc_LcdSegment_t BC_LCD_SEGMENTS[] = {
    {.wsgIndex = BC_WSG_BEAN_UL_00,  .x =   0, .y =  73}, //BC_SEG_BEAN_UL_00
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
    {.wsgIndex = BC_WSG_BEAN_UR_04,  .x = 119, .y = 113}, //BC_SEG_BEAN_UR_04
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
    {.wsgIndex = BC_WSG_BEANBERT_R,  .x = 136, .y = 107}, //BC_SEG_BEANBERT_E
    {.wsgIndex = BC_WSG_SWEARING,    .x = 103, .y =  73}, //BC_SEG_SWEARING
    {.wsgIndex = BC_WSG_BUBBLE_PTR_L,.x = 124, .y =  99}, //BC_SEG_BUBBLE_PTR_L
    {.wsgIndex = BC_WSG_BUBBLE_PTR_R,.x = 142, .y =  99}, //BC_SEG_BUBBLE_PTR_R
    {.wsgIndex = BC_WSG_EGGY_DEVITO, .x = 178, .y =  52}, //BC_SEG_EGGY_DEVITO
    {.wsgIndex = BC_WSG_STRIKE_LBL,  .x = 128, .y =  72}, //BC_SEG_STRIKE_LBL
    {.wsgIndex = BC_WSG_STRIKE_ICON, .x = 119, .y = 188}, //BC_SEG_STRIKE_ICON_00
    {.wsgIndex = BC_WSG_STRIKE_ICON, .x = 137, .y = 188}, //BC_SEG_STRIKE_ICON_01
    {.wsgIndex = BC_WSG_STRIKE_ICON, .x = 155, .y = 188}, //BC_SEG_STRIKE_ICON_02
    {.wsgIndex = BC_WSG_GAME_A_LBL,  .x = 155, .y = 187}, //BC_SEG_GAME_A_LBL
    {.wsgIndex = BC_WSG_GAME_B_LBL,  .x = 217, .y = 187}, //BC_SEG_GAME_B_LBL
    {.wsgIndex = BC_WSG_AM_LABEL,    .x =  21, .y =  53}, //BC_SEG_AM_LABEL
    {.wsgIndex = BC_WSG_ALARM_ICON,  .x = 126, .y =  51}, //BC_SEG_ALARM_ICON
    {.wsgIndex = BC_WSG_ALARM_ANIM_1,.x = 128, .y =  46}, //BC_SEG_ALARM_ANIM_1
    {.wsgIndex = BC_WSG_ALARM_ANIM_2,.x = 127, .y =  39}, //BC_SEG_ALARM_ANIM_2
    {.wsgIndex = BC_WSG_HOURS_SEP,   .x = 67,  .y =  56}  //BC_SEG_HOURS_SEP
};
