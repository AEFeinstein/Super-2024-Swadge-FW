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

static void cg_groveDebug(cGrove_t* cg);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draws the grove in its current state
 *
 * @param cg Game Data
 */
void cg_groveDraw(cGrove_t* cg)
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
    // vec_t cam = {.x = -cg->garden.field.cam.aabb.pos.x, .y = -cg->garden.field.cam.aabb.pos.y};
    for (int8_t c = 0; c < CG_MAX_CHOWA; c++)
    {
        if (cg->chowa[c].active)
        {
            // cgDrawChowa(cg, c, cam);
        }
    }

    // Draw guest Chowa
    for (int8_t c = 0; c < CG_GROVE_MAX_GUEST_CHOWA; c++)
    {
        if (cg->grove.guests[c].active)
        {
            // cgDrawChowa(cg, c, cam);
        }
    }

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

static void cg_groveDebug(cGrove_t* cg)
{
    int16_t xOffset = -cg->grove.camera.pos.x;
    int16_t yOffset = -cg->grove.camera.pos.y;
    // draw AABBs for grove
    for (int32_t i = 0; i < 4; i++)
    {
        drawRect(cg->grove.boundaries[i].pos.x + xOffset, cg->grove.boundaries[i].pos.y + yOffset,
                 cg->grove.boundaries[i].pos.x + cg->grove.boundaries[i].width + xOffset, cg->grove.boundaries[i].pos.y + cg->grove.boundaries[i].height + yOffset, c500);
    }
}