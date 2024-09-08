/**
 * @file cg_chowa.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Pet behavior and appearance
 * @version 0.1
 * @date 2024-09-08
 *
 * @copyright Copyright (c) 2024
 *
 */

// Includes
#include "cg_Chowa.h"

//==============================================================================
// Defines
//==============================================================================

// Body position default values
#define CG_DEFAULT_HEAD_X    16
#define CG_DEFAULT_HEAD_Y    16
#define CG_DEFAULT_HEAD_SIZE 12
#define CG_DEFAULT_BODY_X    16
#define CG_DEFAULT_BODY_Y    32
#define CG_DEFAULT_BODY_SIZE 12
#define CG_DEFAULT_ARM_L_X   8
#define CG_DEFAULT_ARM_L_Y   32
#define CG_DEFAULT_ARM_R_X   24
#define CG_DEFAULT_ARM_R_Y   32
#define CG_DEFAULT_LEG_L_X   8
#define CG_DEFAULT_LEG_L_Y   48
#define CG_DEFAULT_LEG_R_X   24
#define CG_DEFAULT_LEG_R_Y   48
#define CG_DEFAULT_ARM_SIZE  6
#define CG_DEFAULT_LEG_SIZE  6

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    CHOWA_HEAD_MAIN,
    CHOWA_HEAD_SECONDARY,
    CHOWA_BODY_MAIN,
    CHOWA_BODY_SECONDARY,
    CHOWA_HANDS,
    CHOWA_FEET
} cgChowaColorsIdx;

typedef enum
{
    CHOWA_LEFT_HAND,
    CHOWA_RIGHT_HAND,
    CHOWA_LEFT_LEG,
    CHOWA_RIGHT_LEG
} cgChowaLimbs_t;

//==============================================================================
// Static Functions
//==============================================================================

static void _cgDrawBody(cGrove_t* cg, int8_t idx, vec_t offset)
{
    drawCircleFilled(cg->chowa[idx].body.bodyOffset.x + offset.x, cg->chowa[idx].body.bodyOffset.y + offset.y,
                     cg->chowa[idx].body.bodySize, cg->chowa->colors[CHOWA_HEAD_MAIN]);
    drawCircleFilled(cg->chowa[idx].body.bodyOffset.x + offset.x,
                     cg->chowa[idx].body.bodyOffset.y + (cg->chowa[idx].body.bodySize >> 2) + offset.y,
                     cg->chowa[idx].body.bodySize >> 1, cg->chowa->colors[CHOWA_HEAD_SECONDARY]);
    drawCircleOutline(cg->chowa[idx].body.bodyOffset.x + offset.x, cg->chowa[idx].body.bodyOffset.y + offset.y,
                      cg->chowa[idx].body.bodySize, 1, c000);
}

static void _cgDrawHead(cGrove_t* cg, int8_t idx, vec_t offset)
{
    drawCircleFilled(cg->chowa[idx].body.headOffset.x + offset.x, cg->chowa[idx].body.headOffset.y + offset.y,
                     cg->chowa[idx].body.headSize, cg->chowa->colors[CHOWA_BODY_MAIN]);
    drawCircleFilled(cg->chowa[idx].body.headOffset.x + offset.x,
                     cg->chowa[idx].body.headOffset.y - (cg->chowa[idx].body.headSize >> 2) + offset.y,
                     cg->chowa[idx].body.headSize >> 1, cg->chowa->colors[CHOWA_BODY_SECONDARY]);
    drawCircleOutline(cg->chowa[idx].body.headOffset.x + offset.x, cg->chowa[idx].body.headOffset.y + offset.y,
                      cg->chowa[idx].body.headSize, 1, c000);
}

static void _cgDrawLimb(cGrove_t* cg, int8_t idx, vec_t offset, cgChowaLimbs_t limb)
{
    paletteColor_t col = (limb == CHOWA_LEFT_HAND || limb == CHOWA_RIGHT_HAND) ? cg->chowa->colors[CHOWA_HANDS]
                                                                               : cg->chowa->colors[CHOWA_FEET];
    drawCircleFilled(cg->chowa[idx].body.limbRestPos[limb].x + offset.x, cg->chowa[idx].body.limbRestPos[limb].y + offset.y,
                     cg->chowa[idx].body.limbSize[limb], cg->chowa->colors[col]);
    drawCircleOutline(cg->chowa[idx].body.limbRestPos[limb].x + offset.x, cg->chowa[idx].body.limbRestPos[limb].y + offset.y,
                     cg->chowa[idx].body.limbSize[limb], 1, c000);
}

//==============================================================================
// Functions
//==============================================================================

void cgInitChowa(cGrove_t* cg, int8_t idx, paletteColor_t* colors)
{
    for (int i = 0; i < CG_CHOWA_COLORS; i++)
    {
        cg->chowa[idx].colors[i] = colors[i];
    }
    // Body positions
    cg->chowa[idx].body.headOffset.x                    = CG_DEFAULT_HEAD_X;
    cg->chowa[idx].body.headOffset.y                    = CG_DEFAULT_HEAD_Y;
    cg->chowa[idx].body.headSize                        = CG_DEFAULT_HEAD_SIZE;
    cg->chowa[idx].body.bodyOffset.x                    = CG_DEFAULT_BODY_X;
    cg->chowa[idx].body.bodyOffset.y                    = CG_DEFAULT_BODY_Y;
    cg->chowa[idx].body.bodySize                        = CG_DEFAULT_BODY_SIZE;
    cg->chowa[idx].body.limbRestPos[CHOWA_LEFT_HAND].x  = CG_DEFAULT_ARM_L_X;
    cg->chowa[idx].body.limbRestPos[CHOWA_LEFT_HAND].y  = CG_DEFAULT_ARM_L_Y;
    cg->chowa[idx].body.limbRestPos[CHOWA_RIGHT_HAND].x = CG_DEFAULT_ARM_R_X;
    cg->chowa[idx].body.limbRestPos[CHOWA_RIGHT_HAND].y = CG_DEFAULT_ARM_R_Y;
    cg->chowa[idx].body.limbRestPos[CHOWA_LEFT_LEG].x   = CG_DEFAULT_LEG_L_X;
    cg->chowa[idx].body.limbRestPos[CHOWA_LEFT_LEG].y   = CG_DEFAULT_LEG_L_Y;
    cg->chowa[idx].body.limbRestPos[CHOWA_RIGHT_LEG].x  = CG_DEFAULT_LEG_R_X;
    cg->chowa[idx].body.limbRestPos[CHOWA_RIGHT_LEG].y  = CG_DEFAULT_LEG_R_Y;
    cg->chowa[idx].body.limbSize[CHOWA_LEFT_HAND]       = CG_DEFAULT_ARM_SIZE;
    cg->chowa[idx].body.limbSize[CHOWA_RIGHT_HAND]      = CG_DEFAULT_ARM_SIZE;
    cg->chowa[idx].body.limbSize[CHOWA_LEFT_LEG]        = CG_DEFAULT_LEG_SIZE;
    cg->chowa[idx].body.limbSize[CHOWA_RIGHT_LEG]       = CG_DEFAULT_LEG_SIZE;

    // TODO: set up AABB
    cg->chowa->aabb.pos.x = 64;
    cg->chowa->aabb.pos.y = 64;
    // TODO: vary size and shape based on health, age, etc
}

void cgDrawChowa(cGrove_t* cg, int8_t idx, vec_t offset)
{
    // NOTE: Offset should include:
    // - Camera if relevant
    // - Animation offsets (individual to limbs and head)

    // Draw body in neutral position
    _cgDrawBody(cg, idx, offset);
    _cgDrawHead(cg, idx, offset);
    _cgDrawLimb(cg, idx, offset, CHOWA_LEFT_HAND);
    _cgDrawLimb(cg, idx, offset, CHOWA_RIGHT_HAND);
    _cgDrawLimb(cg, idx, offset, CHOWA_LEFT_LEG);
    _cgDrawLimb(cg, idx, offset, CHOWA_RIGHT_LEG);
}