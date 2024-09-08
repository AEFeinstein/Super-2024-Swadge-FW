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
#include "esp_random.h"
#include "cg_Types.h"

//==============================================================================
// Defines
//==============================================================================

// Body position default values
#define CG_DEFAULT_HEAD_X    16
#define CG_DEFAULT_HEAD_Y    12
#define CG_DEFAULT_HEAD_SIZE 12
#define CG_DEFAULT_BODY_X    16
#define CG_DEFAULT_BODY_Y    28
#define CG_DEFAULT_BODY_SIZE 12
#define CG_DEFAULT_ARM_L_X   8
#define CG_DEFAULT_ARM_L_Y   28
#define CG_DEFAULT_ARM_R_X   24
#define CG_DEFAULT_ARM_R_Y   28
#define CG_DEFAULT_LEG_L_X   8
#define CG_DEFAULT_LEG_L_Y   44
#define CG_DEFAULT_LEG_R_X   24
#define CG_DEFAULT_LEG_R_Y   44
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

static void _cgChowaMove(cGrove_t* cg, int8_t idx, vec_t change)
{
    cg->chowa[idx].aabb.pos = addVec2d(cg->chowa[idx].aabb.pos, change);
}

static void _cgDrawBody(cGrove_t* cg, int8_t idx, vec_t offset)
{
    drawCircleFilled(cg->chowa[idx].body.bodyOffset.x + cg->chowa[idx].aabb.pos.x + offset.x,
                     cg->chowa[idx].body.bodyOffset.y + cg->chowa[idx].aabb.pos.y + offset.y,
                     cg->chowa[idx].body.bodySize, cg->chowa->colors[CHOWA_HEAD_MAIN]);
    drawCircleFilled(cg->chowa[idx].body.bodyOffset.x + cg->chowa[idx].aabb.pos.x + offset.x,
                     cg->chowa[idx].body.bodyOffset.y + cg->chowa[idx].aabb.pos.y + (cg->chowa[idx].body.bodySize >> 2)
                         + offset.y,
                     cg->chowa[idx].body.bodySize >> 1, cg->chowa->colors[CHOWA_HEAD_SECONDARY]);
    drawCircleOutline(cg->chowa[idx].body.bodyOffset.x + cg->chowa[idx].aabb.pos.x + offset.x,
                      cg->chowa[idx].body.bodyOffset.y + cg->chowa[idx].aabb.pos.y + offset.y,
                      cg->chowa[idx].body.bodySize, 1, c000);
}

static void _cgDrawHead(cGrove_t* cg, int8_t idx, vec_t offset)
{
    drawCircleFilled(cg->chowa[idx].body.headOffset.x + cg->chowa[idx].aabb.pos.x + offset.x,
                     cg->chowa[idx].body.headOffset.y + cg->chowa[idx].aabb.pos.y + offset.y,
                     cg->chowa[idx].body.headSize, cg->chowa->colors[CHOWA_BODY_MAIN]);
    drawCircleFilled(cg->chowa[idx].body.headOffset.x + cg->chowa[idx].aabb.pos.x + offset.x,
                     cg->chowa[idx].body.headOffset.y + cg->chowa[idx].aabb.pos.y - (cg->chowa[idx].body.headSize >> 2)
                         + offset.y,
                     cg->chowa[idx].body.headSize >> 1, cg->chowa->colors[CHOWA_BODY_SECONDARY]);
    drawCircleOutline(cg->chowa[idx].body.headOffset.x + cg->chowa[idx].aabb.pos.x + offset.x,
                      cg->chowa[idx].body.headOffset.y + cg->chowa[idx].aabb.pos.y + offset.y,
                      cg->chowa[idx].body.headSize, 1, c000);
}

static void _cgDrawLimb(cGrove_t* cg, int8_t idx, vec_t offset, cgChowaLimbs_t limb)
{
    cgChowaLimbs_t col = (limb == CHOWA_LEFT_HAND || limb == CHOWA_RIGHT_HAND) ? CHOWA_HANDS : CHOWA_FEET;
    drawCircleFilled(cg->chowa[idx].body.limbRestPos[limb].x + cg->chowa[idx].aabb.pos.x + offset.x,
                     cg->chowa[idx].body.limbRestPos[limb].y + cg->chowa[idx].aabb.pos.y + offset.y,
                     cg->chowa[idx].body.limbSize[limb], cg->chowa->colors[col]);
    drawCircleOutline(cg->chowa[idx].body.limbRestPos[limb].x + cg->chowa[idx].aabb.pos.x + offset.x,
                      cg->chowa[idx].body.limbRestPos[limb].y + cg->chowa[idx].aabb.pos.y + offset.y,
                      cg->chowa[idx].body.limbSize[limb], 1, c000);
}

static void _cgDrawExpression(cGrove_t* cg, int8_t idx, vec_t offset)
{
    wsg_t currExpresion = cg->chowaExpressions[cg->chowa[idx].mood];
    drawWsgSimple(&currExpresion,
                  cg->chowa[idx].body.headOffset.x + cg->chowa[idx].aabb.pos.x + offset.x - (currExpresion.w / 2),
                  cg->chowa[idx].body.headOffset.y + cg->chowa[idx].aabb.pos.y + offset.y - (currExpresion.h / 2));
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
    // TODO: vary size and shape based on health, age, etc
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

    // AABB
    // FIXME: properly set up AABB. Conform to size of Chowa
    cg->chowa[idx].aabb.pos.x  = 128;
    cg->chowa[idx].aabb.pos.y  = 128;
    cg->chowa[idx].aabb.height = 48;
    cg->chowa[idx].aabb.width  = 32;

    // Other
    cg->chowa->mood   = CG_HAPPY;
    cg->chowa->active = true;
    cg->chowa->gState = CHOWA_STATIC;
}

void cgChowaGardenAI(cGrove_t* cg, int8_t idx)
{
    // TODO: Have mood have more effect on what states can be entered
    switch (cg->chowa[idx].gState)
    {
        case CHOWA_STATIC:
            if (cg->chowa[idx].waitTimer > 0)
            {
                cg->chowa[idx].waitTimer -= 1;
            }
            else
            {
                switch (esp_random() % 3) // FIXME: Change to picking only behaviors that are possible to pick
                {
                    case CHOWA_STATIC:
                        cg->chowa[idx].waitTimer = 120;
                        break;
                    case CHOWA_WANDER:
                        cg->chowa[idx].mood = CG_NEUTRAL;
                        cg->chowa[idx].targetPos.x
                            = CG_FIELD_BOUNDARY + esp_random() % (CG_FIELD_WIDTH - (2 * CG_FIELD_BOUNDARY));
                        cg->chowa[idx].targetPos.y
                            = CG_FIELD_BOUNDARY + esp_random() % (CG_FIELD_HEIGHT - (2 * CG_FIELD_BOUNDARY));
                        break;
                    case CHOWA_CHASE:
                        cg->chowa[idx].mood = CG_HAPPY;
                        cg->chowa[idx].targetPos.x
                            = CG_FIELD_BOUNDARY + esp_random() % (CG_FIELD_WIDTH - (2 * CG_FIELD_BOUNDARY));
                        cg->chowa[idx].targetPos.y
                            = CG_FIELD_BOUNDARY + esp_random() % (CG_FIELD_HEIGHT - (2 * CG_FIELD_BOUNDARY));
                        break;
                    default:
                        break;
                }
            }
            break;
        case CHOWA_WANDER:
        case CHOWA_CHASE:
            // Check distance to target
            // If at wander point, go static
            // else, move toward wander point
            break;
        case CHOWA_HELD:
            // Struggle animation
            break;
        default:
            break;
    }
}

void cgDrawChowa(cGrove_t* cg, int8_t idx, vec_t offset)
{
    // TODO: Create Animation offsets

    // Draw body
    _cgDrawBody(cg, idx, offset);
    _cgDrawHead(cg, idx, offset);
    _cgDrawExpression(cg, idx, offset);
    _cgDrawLimb(cg, idx, offset, CHOWA_LEFT_HAND);
    _cgDrawLimb(cg, idx, offset, CHOWA_RIGHT_HAND);
    _cgDrawLimb(cg, idx, offset, CHOWA_LEFT_LEG);
    _cgDrawLimb(cg, idx, offset, CHOWA_RIGHT_LEG);

    drawRect(offset.x + cg->chowa[idx].aabb.pos.x, offset.y + cg->chowa[idx].aabb.pos.y,
             offset.x + cg->chowa[idx].aabb.pos.x + cg->chowa[idx].aabb.width,
             offset.y + cg->chowa[idx].aabb.pos.y + cg->chowa[idx].aabb.height, c500);
}