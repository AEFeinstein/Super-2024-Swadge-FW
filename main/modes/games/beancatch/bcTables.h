#ifndef BEANCATCH_TABLES_INCLUDED
#define BEANCATCH_TABLES_INCLUDED

//==============================================================================
// Includes
//==============================================================================
#include <inttypes.h>
#include "cnfs_image.h"

//==============================================================================
// Constants
//==============================================================================
#define BC_WSG_SIZE 44
#define BC_SEGMENTS_SIZE 43
#define BC_DIFFICULTY_MAX_BEANS_SIZE 18

//==============================================================================
// Macros
//==============================================================================

//==============================================================================
// Typedefs
//==============================================================================

typedef struct beancatch_t beancatch_t;

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
    uint16_t wsgIndex;
    uint32_t x;
    uint32_t y;
} bc_LcdSegment_t;

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    BC_ST_NULL,
    BC_ST_ACL,
    BC_ST_CLOCK,
    BC_ST_GAME_A,
    BC_ST_GAME_B,
    BC_ST_GAME_OVER
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

typedef enum
{
    BC_CONVEYOR_NULL = -1,
    BC_CONVEYOR_LU,
    BC_CONVEYOR_RU,
    BC_CONVEYOR_LD,
    BC_CONVEYOR_RD
} bc_conveyorIndex_t;

//==============================================================================
// Extern Variables
//==============================================================================

extern const cnfsFileIdx_t BC_WSGS[];
extern const bc_LcdSegment_t BC_LCD_SEGMENTS[BC_SEGMENTS_SIZE];
extern const uint16_t BC_DIFFICULTY_MAX_BEANS[BC_DIFFICULTY_MAX_BEANS_SIZE];

#endif