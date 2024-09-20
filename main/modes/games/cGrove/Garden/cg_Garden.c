/**
 * @file cg_Garden.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief The main interation area with the Chowa
 * @version 0.1
 * @date 2024-09-07
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "cg_Garden.h"
#include "cg_Field.h"
#include "cg_Items.h"

//==============================================================================
// Static functions
//==============================================================================

/**
 * @brief Draws the hand at the appropriate position
 *
 * @param cg Game object
 */
static void _cgDrawHand(cGrove_t* cg)
{
    drawWsgSimple(&cg->cursors[0], cg->garden.cursorAABB.pos.x, cg->garden.cursorAABB.pos.y);
}

/**
 * @brief Attempts to grab objects. Due to total amount being limited, no need to optimize
 *
 * @param cg Game Object
 */
static void _cgAttemptGrab(cGrove_t* cg)
{
    vec_t collVec;
    cg->garden.heldItem = NULL;
    // Check if over a Chowa
    for (int8_t c = 0; c < CG_MAX_CHOWA; c++)
    {
        if (cg->chowa[c].active)
        {
            rectangle_t translated = {.pos = subVec2d(cg->chowa[c].aabb.pos, cg->garden.field.cam.aabb.pos),
                                      .height = cg->chowa[c].aabb.height,
                                      .width  = cg->chowa[c].aabb.width};
            if (rectRectIntersection(cg->garden.cursorAABB, translated, &collVec))
            {
                cg->garden.holdingChowa = true;
                cg->garden.heldChowa    = &cg->chowa[c];
                cg->chowa[c].mood       = CG_WORRIED;
            }
        }
    }
    // Check if over an item
    for (int8_t item = 0; item < CG_FIELD_ITEM_LIMIT; item++)
    {
        if (cg->garden.items[item].active)
        {
            rectangle_t translated = {.pos = subVec2d(cg->garden.items[item].aabb.pos, cg->garden.field.cam.aabb.pos),
                                      .height = cg->garden.items[item].aabb.height,
                                      .width  = cg->garden.items[item].aabb.width};
            if (rectRectIntersection(cg->garden.cursorAABB, translated, &collVec))
            {
                cg->garden.holdingItem = true;
                cg->garden.heldItem    = &cg->garden.items[item];
            }
        }
    }
}

/**
 * @brief Input handling for garden
 *
 * @param cg Game Object
 */
static void _cgHandleInputGarden(cGrove_t* cg)
{
    buttonEvt_t evt;
    // Touch pad for the hands
    if (cg->touch)
    {
        int32_t phi, r, intensity;
        if (getTouchJoystick(&phi, &r, &intensity))
        {
            int16_t speed = phi >> 5;
            if (!(speed <= 5))
            {
                printf("touch center: %" PRIu32 ", intensity: %" PRIu32 ", intensity %" PRIu32 "\n", phi, r, intensity);
                // Move hand
                cg->garden.cursorAABB.pos.x += (getCos1024(phi) * speed) / 1024;
                cg->garden.cursorAABB.pos.y -= (getSin1024(phi) * speed) / 1024;
            }
        }
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.button & PB_A && evt.down)
            {
                if (cg->garden.holdingItem || cg->garden.holdingChowa)
                {
                    cg->garden.holdingItem  = false;
                    cg->garden.holdingChowa = false;
                }
                else
                {
                    _cgAttemptGrab(cg);
                }
            }
            if (evt.button & PB_B && evt.down)
            {
                // TODO: Pet Chowa
            }
        }
    }
    else
    {
        while (checkButtonQueueWrapper(&evt))
        {
            if (evt.button & PB_RIGHT)
            {
                cg->garden.cursorAABB.pos.x += CG_CURSOR_SPEED;
            }
            else if (evt.button & PB_LEFT)
            {
                cg->garden.cursorAABB.pos.x -= CG_CURSOR_SPEED;
            }
            if (evt.button & PB_UP)
            {
                cg->garden.cursorAABB.pos.y -= CG_CURSOR_SPEED;
            }
            else if (evt.button & PB_DOWN)
            {
                cg->garden.cursorAABB.pos.y += CG_CURSOR_SPEED;
            }
            if (evt.button & PB_A && evt.down)
            {
                if (cg->garden.holdingItem || cg->garden.holdingChowa)
                {
                    cg->garden.holdingItem  = false;
                    cg->garden.holdingChowa = false;
                }
                else
                {
                    _cgAttemptGrab(cg);
                }
            }
            if (evt.button & PB_B && evt.down)
            {
                // TODO: Pet Chowa
            }
        }
    }
    // Check if out of bounds
    if (cg->garden.cursorAABB.pos.x < CG_FIELD_BOUNDARY)
    {
        cgMoveCamera(cg, -CG_CURSOR_SPEED, 0);
        cg->garden.cursorAABB.pos.x = CG_FIELD_BOUNDARY;
    }
    else if (cg->garden.cursorAABB.pos.x > TFT_WIDTH - (CG_FIELD_BOUNDARY + cg->garden.cursorAABB.width))
    {
        cgMoveCamera(cg, CG_CURSOR_SPEED, 0);
        cg->garden.cursorAABB.pos.x = TFT_WIDTH - (CG_FIELD_BOUNDARY + cg->garden.cursorAABB.width);
    }
    if (cg->garden.cursorAABB.pos.y < CG_FIELD_BOUNDARY)
    {
        cgMoveCamera(cg, 0, -CG_CURSOR_SPEED);
        cg->garden.cursorAABB.pos.y = CG_FIELD_BOUNDARY;
    }
    else if (cg->garden.cursorAABB.pos.y > TFT_HEIGHT - (CG_FIELD_BOUNDARY + cg->garden.cursorAABB.height))
    {
        cgMoveCamera(cg, 0, CG_CURSOR_SPEED);
        cg->garden.cursorAABB.pos.y = TFT_HEIGHT - (CG_FIELD_BOUNDARY + cg->garden.cursorAABB.height);
    }
}

//==============================================================================
// Functions
//==============================================================================

void cgInitGarden(cGrove_t* cg)
{
    // Initialize the cursor
    cg->garden.cursorAABB.height = 32;
    cg->garden.cursorAABB.width  = 32;
    cg->garden.cursorAABB.pos.x  = 32;
    cg->garden.cursorAABB.pos.y  = 32;
    cg->garden.holdingChowa      = false;
    cg->garden.holdingItem       = false;

    // Initialize the items
    vec_t pos;
    pos.x = 64;
    pos.y = 64;
    cgInitItem(cg, 0, "Ball", cg->items[0], pos);

    // Initialize the field
    cgInitField(cg);
}

void cgRunGarden(cGrove_t* cg)
{
    // Input
    _cgHandleInputGarden(cg);

    // Garden Logic
    if (cg->garden.holdingItem)
    {
        cg->garden.heldItem->aabb.pos = addVec2d(cg->garden.cursorAABB.pos, cg->garden.field.cam.aabb.pos);
    }
    if (cg->garden.holdingChowa)
    {
        cg->garden.heldChowa->aabb.pos = addVec2d(cg->garden.cursorAABB.pos, cg->garden.field.cam.aabb.pos);
    }

    // TODO: Chowa AI
    // - Wander
    // - Chase ball
    // - struggle in grip

    // Draw
    // Field
    cgDrawField(cg);
    // Items
    for (int8_t item = 0; item < CG_FIELD_ITEM_LIMIT; item++)
    {
        if (cg->garden.items[item].active)
        {
            cgDrawItem(cg, item);
        }
    }
    // Chowa
    //vec_t cam = {.x = -cg->garden.field.cam.aabb.pos.x, .y = -cg->garden.field.cam.aabb.pos.y};
    for (int8_t c = 0; c < CG_FIELD_ITEM_LIMIT; c++)
    {
        if (cg->chowa[c].active)
        {
            //cgDrawChowa(cg, c, cam);
        }
    }
    // Hand
    _cgDrawHand(cg);
}