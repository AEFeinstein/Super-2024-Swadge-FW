/**
 * @file cg_GroveDraw.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Drawing functions for the Grove mode of Chowa Grove
 * @version 0.1
 * @date 2024-10-09
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "cg_GroveDraw.h"
#include "cg_Chowa.h"
#include <esp_random.h>

//==============================================================================
// Defines
//==============================================================================

#define SECOND 1000000

//==============================================================================
// Function declarations
//==============================================================================

/**
 * @brief Draws the hand at the appropriate position
 *
 * @param cg Game object
 */
static void cg_drawHand(cGrove_t* cg);

/**
 * @brief Draws item at specified index
 *
 * @param cg Game Object
 * @param idx index of item to draw
 */
static void cg_drawItem(cGrove_t* cg, int8_t idx);

/**
 * @brief Draws a Chowa
 *
 * @param cg Game Data
 * @param elapsedUS Time since last frame
 */
static void cg_drawChowaGrove(cGrove_t* cg, int64_t elapsedUS);

/**
 * @brief Debug visualization
 *
 * @param cg Game Data
 */
static void cg_groveDebug(cGrove_t* cg);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draws the grove in its current state
 *
 * @param cg Game Data
 */
void cg_groveDraw(cGrove_t* cg, int64_t elapsedUs)
{
    // Get camera offset

    // Draw BG
    drawWsgSimple(&cg->grove.groveBG, -cg->grove.camera.pos.x, -cg->grove.camera.pos.y);

    // Draw Items
    for (int8_t item = 0; item < CG_GROVE_MAX_ITEMS; item++)
    {
        if (cg->grove.items[item].active)
        {
            cg_drawItem(cg, item);
        }
    }

    // Draw Chowa
    cg_drawChowaGrove(cg, elapsedUs);

    // Draw UI
    cg_drawHand(cg);

    // Debug draw
    cg_groveDebug(cg);
}

//==============================================================================
// Static Functions
//==============================================================================

static void cg_drawHand(cGrove_t* cg)
{
    drawWsgSimple(&cg->grove.cursors[0], cg->grove.cursor.pos.x, cg->grove.cursor.pos.y);
}

static void cg_drawItem(cGrove_t* cg, int8_t idx)
{
    int16_t xOffset = cg->grove.items[idx].aabb.pos.x - cg->grove.camera.pos.x;
    int16_t yOffset = cg->grove.items[idx].aabb.pos.y - cg->grove.camera.pos.y;
    drawWsgSimple(&cg->grove.items[idx].spr, xOffset, yOffset);
    drawText(&cg->menuFont, c555, cg->grove.items[idx].name, xOffset, yOffset - 16);
}

static void cg_drawChowaGrove(cGrove_t* cg, int64_t elapsedUS)
{
    for (int idx = 0; idx < CG_MAX_CHOWA + CG_GROVE_MAX_GUEST_CHOWA; idx++)
    {
        cgGroveChowa_t* c = &cg->grove.chowa[idx];
        if (!c->chowa->active)
        {
            continue;
        }
        int16_t xOffset = c->aabb.pos.x - cg->grove.camera.pos.x;
        int16_t yOffset = c->aabb.pos.y - cg->grove.camera.pos.y;
        wsg_t* spr;
        switch (c->gState)
        {
            case CHOWA_STATIC:
            {
                // Update animation frame if enough time has passed
                c->frameTimer += elapsedUS;
                if (c->frameTimer > SECOND / 2)
                {
                    c->frameTimer = 0;
                    c->animFrame  = (c->animFrame + 1) % 2;
                }
                // Pick option
                if (c->chowa->mood == CG_ANGRY)
                {
                    spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_ANGRY, c->animFrame);
                    drawWsgSimple(spr, xOffset, yOffset);
                    if (c->animFrame == 0)
                    {
                        drawWsgSimple(&cg->grove.angerParticles[1], xOffset, yOffset);
                    }
                    else
                    {
                        drawWsg(&cg->grove.angerParticles[1], xOffset + 17, yOffset, true, false, 0);
                        drawWsgSimple(&cg->grove.angerParticles[0], xOffset + 5, yOffset + 3);
                    }
                }
                else if (c->chowa->mood == CG_SAD)
                {
                    spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_SAD, c->animFrame);
                    drawWsgSimple(spr, xOffset, yOffset);
                }
                else if (c->chowa->mood == CG_HAPPY)
                {
                    spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_PET, 0);
                    drawWsgSimple(spr, xOffset, yOffset);
                }
                else if (c->chowa->mood == CG_CONFUSED)
                {
                    spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_SIT, 0);
                    drawWsgSimple(spr, xOffset, yOffset);
                    switch (c->chowa->type)
                    {
                        case CG_RED_LUMBERJACK:
                        default:
                        {
                            drawWsgSimple(&cg->grove.questionMarks[0], xOffset + 5, yOffset - 10);
                            break;
                        }
                        case CG_GREEN_LUMBERJACK:
                        {
                            drawWsgSimple(&cg->grove.questionMarks[1], xOffset + 5, yOffset - 10);
                            break;
                        }
                        case CG_CHO:
                        {
                            drawWsgSimple(&cg->grove.questionMarks[2], xOffset + 5, yOffset - 10);
                            break;
                        }
                        case CG_LILB:
                        {
                            drawWsgSimple(&cg->grove.questionMarks[3], xOffset + 5, yOffset - 10);
                            break;
                        }
                        case CG_KOSMO:
                        {
                            drawWsgSimple(&cg->grove.questionMarks[4], xOffset + 5, yOffset - 10);
                            break;
                        }
                        case CG_KING_DONUT:
                        {
                            drawWsgSimple(&cg->grove.questionMarks[5], xOffset + 5, yOffset - 10);
                            break;
                        }
                    }
                }
                else
                {
                    spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_SIT, 0);
                    drawWsgSimple(spr, xOffset, yOffset);
                }
                break;
            }
            case CHOWA_WALK:
            {
                // Update animation frame if enough time has passed
                c->frameTimer += elapsedUS;
                if (c->frameTimer > SECOND / 4)
                {
                    c->frameTimer = 0;
                    c->animFrame  = (c->animFrame + 1) % 4;
                }
                bool flip = false;
                vec_t temp;
                // Check if in the water
                if (rectRectIntersection(c->aabb, cg->grove.boundaries[CG_WATER], &temp))
                {
                    if (c->angle <= 270 && c->angle > 90)
                    {
                        flip = true;
                    }
                    spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_SWIM, c->animFrame);
                }
                else
                {
                    if (c->angle > 45 && c->angle <= 135)
                    {
                        spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_WALK_DOWN, c->animFrame);
                    }
                    else if (c->angle > 135 && c->angle <= 225)
                    {
                        spr  = cg_getChowaWSG(cg, c->chowa, CG_ANIM_WALK_SIDE, c->animFrame);
                        flip = true;
                    }
                    else if (c->angle > 225 && c->angle <= 315)
                    {
                        spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_WALK_UP, c->animFrame);
                    }
                    else
                    {
                        spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_WALK_SIDE, c->animFrame);
                    }
                }
                drawWsg(spr, xOffset, yOffset, flip, false, 0);
                break;
            }
            case CHOWA_SING:
            {
                // Update animation frame if enough time has passed
                c->frameTimer += elapsedUS;
                if (c->frameTimer > SECOND / 4)
                {
                    c->frameTimer = 0;
                    c->animFrame  = (c->animFrame + 1) % 4;
                }
                spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_SING, c->animFrame);
                drawWsgSimple(spr, xOffset, yOffset);
                // TODO: Add music note sprites
                break;
            }
            case CHOWA_DANCE:
            {
                // Update animation frame if enough time has passed
                c->frameTimer += elapsedUS;
                if (c->frameTimer > SECOND / 4)
                {
                    c->frameTimer = 0;
                    c->animFrame  = (c->animFrame + 1) % 4;
                }
                spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_DANCE, c->animFrame);
                drawWsgSimple(spr, xOffset, yOffset);
                break;
            }
            case CHOWA_BOX:
            {
                // Update animation frame if enough time has passed
                c->frameTimer += elapsedUS;
                if (c->frameTimer > SECOND / 4)
                {
                    c->frameTimer = 0;
                    c->animFrame  = (c->animFrame + 1) % 3;
                    if (c->animFrame == 2)
                    {
                        c->animIdx   = esp_random() % 3;
                        c->animFrame = 0;
                    }
                }
                switch (c->animIdx)
                {
                    case 0:
                    {
                        spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_PUNCH, c->animFrame);
                        break;
                    }
                    case 1:
                    {
                        spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_KICK, c->animFrame);
                        break;
                    }
                    case 2:
                    {
                        spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_HEADBUTT, c->animFrame);
                        break;
                    }
                }
                drawWsg(spr, xOffset, yOffset, c->flip, false, 0);
                if (c->hasPartner && c->animFrame % 2 == 0)
                {
                    if (c->flip)
                    {
                        drawWsg(&cg->grove.angerParticles[1], xOffset, yOffset, true, false, 0);
                    }
                    else
                    {
                        drawWsgSimple(&cg->grove.angerParticles[1], xOffset, yOffset);
                    }
                }
                break;
            }
            case CHOWA_HELD:
            {
                // Update animation frame if enough time has passed
                c->frameTimer += elapsedUS;
                if (c->frameTimer > SECOND / 6)
                {
                    c->frameTimer = 0;
                    c->animFrame  = (c->animFrame + 1) % 2;
                }
                spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_FLAIL, c->animFrame);
                break;
            }
            case CHOWA_TALK:
            {
                // Update animation frame if enough time has passed
                c->frameTimer += elapsedUS;
                if (c->frameTimer > SECOND / 6)
                {
                    c->frameTimer = 0;
                    c->animFrame  = (c->animFrame + 1) % 2;
                }
                spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_WALK_SIDE, 0);
                drawWsg(spr, xOffset, yOffset, c->flip, false, 0);
                // Draw Speech bubbles. Only animate if talking to other Chowa
                break;
            }
            default:
            {
                spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_WALK_DOWN, 0);
                break;
            }
        }
    }
}

static void cg_groveDebug(cGrove_t* cg)
{
    int16_t xOffset = -cg->grove.camera.pos.x;
    int16_t yOffset = -cg->grove.camera.pos.y;
    char buffer[32];
    // draw AABBs for grove
    for (int32_t i = 0; i < 3; i++)
    {
        drawRect(cg->grove.boundaries[i].pos.x + xOffset, cg->grove.boundaries[i].pos.y + yOffset,
                 cg->grove.boundaries[i].pos.x + cg->grove.boundaries[i].width + xOffset,
                 cg->grove.boundaries[i].pos.y + cg->grove.boundaries[i].height + yOffset, c500);
    }
    for (int32_t i = 0; i < CG_MAX_CHOWA + CG_GROVE_MAX_GUEST_CHOWA; i++)
    {
        // Draw Chowa info
        if (cg->grove.chowa[i].chowa->active)
        {
            drawRect(cg->grove.chowa[i].aabb.pos.x + xOffset, cg->grove.chowa[i].aabb.pos.y + yOffset,
                     cg->grove.chowa[i].aabb.pos.x + cg->grove.chowa[i].aabb.width + xOffset,
                     cg->grove.chowa[i].aabb.pos.y + cg->grove.chowa[i].aabb.height + yOffset, c500);
            drawCircle(cg->grove.chowa[i].targetPos.x + xOffset, cg->grove.chowa[i].targetPos.y + yOffset, 12, c500);
            drawText(&cg->menuFont, c550, buffer, 24, 24);
        }
    }
}