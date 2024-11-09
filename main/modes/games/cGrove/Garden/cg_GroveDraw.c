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
#include "cg_GroveItems.h"
#include <esp_random.h>

//==============================================================================
// Defines
//==============================================================================

#define SECOND 1000000

//==============================================================================
// Consts
//==============================================================================

static const char pressAB[]        = "Press A to buy, press B to sell";
static const char shopText[]       = "Chowa Gray Market";
static const char invText[]        = "Inventory";
static const char statText[]       = "Chowa Stats";
static const char kickChowa[]      = "Press A to kick this Chowa";
static const char slotEmpty[]      = "No Chowa in this slot";
static const char confirmDefault[] = "Press A to go back, press B to confirm";
static const char abandonChowa[]   = "Abandon Chowa";

//==============================================================================
// Variables
//==============================================================================

static char confirmKick[]    = "Are you sure you want to kick this Chowa?";
static char confirmAbandon[] = "Are you sure your heart is black enough to kill this Chowa?";

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
 * @brief Draws the ring
 *
 * @param cg Game Data
 */
static void cg_drawRing(cGrove_t* cg);

/**
 * @brief Draws a Chowa
 *
 * @param cg Game Data
 * @param elapsedUS Time since last frame
 */
static void cg_drawChowaGrove(cGrove_t* cg, int64_t elapsedUS);

/**
 * @brief Draws the UI elements
 *
 * @param cg Game Data
 */
static void cg_drawUI(cGrove_t* cg);

/**
 * @brief Draw items in a menu
 *
 * @param cg Game Data
 */
static void cg_drawItemsMenu(cGrove_t* cg);

/**
 * @brief Draws a confirmation box in the middle of the screen with a yes/no prompt
 *
 * @param cg Game Date
 * @param string String to print to screen
 */
static void cg_drawConfirmBox(cGrove_t* cg, char* string);

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
void cg_groveDrawField(cGrove_t* cg, int64_t elapsedUs)
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

    cg_drawRing(cg);

    // Draw Chowa
    cg_drawChowaGrove(cg, elapsedUs);

    // Draw UI
    cg_drawHand(cg);
    cg_drawUI(cg);

    // Debug draw
    cg_groveDebug(cg);
}

/**
 * @brief Draws the Shop menu
 *
 * @param cg Game Data
 */
void cg_groveDrawShop(cGrove_t* cg)
{
    char buffer[32];
    // Draw BG
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_WIDTH, c111);

    // Draw Currently selected item and name
    cg_drawItemsMenu(cg);

    // Draw arrows
    int16_t xOffset = (TFT_WIDTH - cg->arrow.w) / 2;
    drawWsg(&cg->arrow, xOffset, 10, false, false, 0);
    drawWsg(&cg->arrow, xOffset, TFT_HEIGHT - (cg->arrow.h + 10), false, true, 0);

    // Draw title
    drawText(&cg->largeMenuFont, c444, shopText, (TFT_WIDTH - textWidth(&cg->largeMenuFont, shopText)) >> 1, 30);

    // Draw cost
    snprintf(buffer, sizeof(buffer) - 1, "Price: %" PRId16, itemPrices[cg->grove.shopSelection]);
    drawText(&cg->menuFont, c555, buffer, 170, 155);

    // Draw current rings
    paletteColor_t color;
    if (cg->grove.inv.money < itemPrices[cg->grove.shopSelection])
    {
        color = c500;
    }
    else
    {
        color = c555;
    }
    drawWsgSimple(&cg->grove.itemsWSGs[11], 12, 165);
    snprintf(buffer, sizeof(buffer) - 1, "Donut: %" PRId32, cg->grove.inv.money);
    drawText(&cg->menuFont, color, buffer, 44, 175);

    // Draw current inv count
    snprintf(buffer, sizeof(buffer) - 1, "Owned: %" PRId16, cg->grove.inv.quantities[cg->grove.shopSelection]);
    drawText(&cg->menuFont, c555, buffer, 170, 175);

    // Draw help text
    drawText(&cg->menuFont, c550, pressAB, 16, 195);
}

/**
 * @brief Draws the inventory
 *
 * @param cg Game Data
 */
void cg_groveDrawInv(cGrove_t* cg)
{
    // Draw BG
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);

    // Draw Title
    drawText(&cg->largeMenuFont, c225, invText, (TFT_WIDTH - textWidth(&cg->largeMenuFont, invText)) >> 1, 32);

    // Draw Currently selected item and name
    cg_drawItemsMenu(cg);

    // Draw arrows
    int16_t xOffset = (TFT_WIDTH - cg->arrow.w) / 2;
    drawWsg(&cg->arrow, xOffset, 10, false, false, 0);
    drawWsg(&cg->arrow, xOffset, TFT_HEIGHT - (cg->arrow.h + 10), false, true, 0);

    // Draw owned quantity
    char buffer[32];
    snprintf(buffer, sizeof(buffer) - 1, "Owned: %" PRId16, cg->grove.inv.quantities[cg->grove.shopSelection]);
    drawText(&cg->menuFont, c555, buffer, 16, 175);

    // Draw qty on field
    int8_t qty = 0;
    for (int idx = 0; idx < CG_GROVE_MAX_ITEMS; idx++)
    {
        if (strcmp(shopMenuItems[cg->grove.shopSelection], cg->grove.items[idx].name) == 0)
        {
            qty++;
        }
    }
    snprintf(buffer, sizeof(buffer) - 1, "Number on field: %" PRId16, qty);
    drawText(&cg->menuFont, c555, buffer, 16, 195);
}

/**
 * @brief Draws the stats of the Chowa currently active
 * 
 * @param cg Game Data
 */
void cg_groveDrawStats(cGrove_t* cg)
{
    // Draw BG
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c010);

    // Title
    drawText(&cg->largeMenuFont, c444, statText, 16, 26);

    // Draw arrows
    int16_t xOffset = (TFT_WIDTH - cg->arrow.w) >> 1;
    drawWsg(&cg->arrow, xOffset, 10, false, false, 0);
    drawWsg(&cg->arrow, xOffset, TFT_HEIGHT - (cg->arrow.h + 10), false, true, 0);

    // Get Chowa
    cgChowa_t* c;
    if (cg->grove.shopSelection > CG_MAX_CHOWA)
    {
        // Is a guest
        c = &cg->guests[cg->grove.shopSelection - CG_MAX_CHOWA];
    }
    else
    {
        c = &cg->chowa[cg->grove.shopSelection];
    }

    // Draw Chowa
    if (c->active)
    {
        wsg_t* spr;
        spr = cg_getChowaWSG(cg, c, CG_ANIM_WALK_DOWN, 0);
        drawText(&cg->largeMenuFont, c555, c->name, 16, 50);
        drawWsgSimpleScaled(spr, 20, 68, 3, 3);

        // Draw stats
        char buffer[32];
        snprintf(buffer, sizeof(buffer) - 1, "Owner: %s", c->owner);
        drawText(&cg->menuFont, c555, buffer, 110, 75);

        // Mood
        switch (c->mood)
        {
            case CG_HAPPY:
            {
                drawText(&cg->menuFont, c555, "Mood: Happy", 110, 90);
                break;
            }
            case CG_WORRIED:
            {
                drawText(&cg->menuFont, c555, "Mood: Worried", 110, 90);
                break;
            }
            case CG_SAD:
            {
                drawText(&cg->menuFont, c555, "Mood: Sad", 110, 90);
                break;
            }
            case CG_ANGRY:
            {
                drawText(&cg->menuFont, c555, "Mood: Angry", 110, 90);
                break;
            }
            case CG_CONFUSED:
            {
                drawText(&cg->menuFont, c555, "Mood: Confused", 110, 90);
                break;
            }
            case CG_SURPRISED:
            {
                drawText(&cg->menuFont, c555, "Mood: Surprised", 110, 90);
                break;
            }
            case CG_SICK:
            {
                drawText(&cg->menuFont, c555, "Mood: Sick", 110, 90);
                break;
            }
            case CG_NEUTRAL:
            default:
            {
                drawText(&cg->menuFont, c555, "Mood: Neutral", 110, 90);
            }
        }

        // Age
        if (c->age > CG_ADULT_AGE)
        {
            drawText(&cg->menuFont, c555, "Age: Adult", 110, 105);
        }
        else
        {
            drawText(&cg->menuFont, c555, "Age: Child", 110, 105);
        }

        // Stats
        snprintf(buffer, sizeof(buffer) - 1, "Agility: %d", c->stats[CG_AGILITY]);
        drawText(&cg->menuFont, c555, buffer, 110, 120);
        snprintf(buffer, sizeof(buffer) - 1, "Charisma: %d", c->stats[CG_CHARISMA]);
        drawText(&cg->menuFont, c555, buffer, 110, 135);
        snprintf(buffer, sizeof(buffer) - 1, "Speed: %d", c->stats[CG_SPEED]);
        drawText(&cg->menuFont, c555, buffer, 110, 150);
        snprintf(buffer, sizeof(buffer) - 1, "Stamina: %d", c->stats[CG_STAMINA]);
        drawText(&cg->menuFont, c555, buffer, 110, 165);
        snprintf(buffer, sizeof(buffer) - 1, "Strength: %d", c->stats[CG_STRENGTH]);
        drawText(&cg->menuFont, c555, buffer, 110, 180);
        snprintf(buffer, sizeof(buffer) - 1, "HP: %d", c->stats[CG_HEALTH]);
        drawText(&cg->menuFont, c555, buffer, 200, 120);

        // Draw kick text for guests
        if (cg->grove.shopSelection > CG_MAX_CHOWA)
        {
            drawText(&cg->menuFont, c550, kickChowa, 16, 195);
        }
    }
    else
    {
        // Chowa is inactive
        drawText(&cg->largeMenuFont, c555, slotEmpty, (TFT_WIDTH - textWidth(&cg->largeMenuFont, slotEmpty)) >> 1,
                 (TFT_HEIGHT - cg->largeMenuFont.height) >> 1);
    }

    // Draw confirmation box
    if (cg->grove.confirm)
    {
        cg_drawConfirmBox(cg, confirmKick);
    }
}

/**
 * @brief Draws the abandon Chowa screen. You monster.
 *
 * @param cg Game Data
 */
void cg_groveDrawAbandon(cGrove_t* cg)
{
    // Draw BG
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c001);

    // Draw Title
    drawText(&cg->largeMenuFont, c225, abandonChowa, (TFT_WIDTH - textWidth(&cg->largeMenuFont, abandonChowa)) >> 1,
             32);

    // Draw Currently selected Chowa
    cgChowa_t* c = &cg->chowa[cg->grove.shopSelection];
    if (c->active)
    {
        wsg_t* spr;
        spr = cg_getChowaWSG(cg, c, CG_ANIM_SAD, 0);
        drawText(&cg->largeMenuFont, c555, c->name, (TFT_WIDTH - textWidth(&cg->largeMenuFont, c->name)) >> 1, 50);
        drawWsgSimpleScaled(spr, (TFT_WIDTH - (spr->h * 3)) >> 1, 75, 3, 3);

        // Draw arrows
        int16_t xOffset = (TFT_WIDTH - cg->arrow.w) >> 1;
        drawWsg(&cg->arrow, xOffset, 10, false, false, 0);
        drawWsg(&cg->arrow, xOffset, TFT_HEIGHT - (cg->arrow.h + 10), false, true, 0);
    }
    else
    {
        // Chowa is inactive
        drawText(&cg->largeMenuFont, c555, slotEmpty, (TFT_WIDTH - textWidth(&cg->largeMenuFont, slotEmpty)) >> 1,
                 (TFT_HEIGHT - cg->largeMenuFont.height) >> 1);
    }

    // Draw confirmation box
    if (cg->grove.confirm)
    {
        cg_drawConfirmBox(cg, confirmAbandon);
    }
}

//==============================================================================
// Static Functions
//==============================================================================

static void cg_drawHand(cGrove_t* cg)
{
    // If holding
    if (cg->grove.holdingChowa || cg->grove.holdingItem)
    {
        drawWsgSimple(&cg->grove.cursors[2], cg->grove.cursor.pos.x, cg->grove.cursor.pos.y);
        return;
    }
    vec_t temp;
    rectangle_t rect = {.pos    = addVec2d(cg->grove.cursor.pos, cg->grove.camera.pos),
                        .height = cg->grove.cursor.height,
                        .width  = cg->grove.cursor.width};
    // If hovering over
    for (int idx = 0; idx < CG_MAX_CHOWA + CG_GROVE_MAX_GUEST_CHOWA; idx++)
    {
        // Chowa
        if (rectRectIntersection(rect, cg->grove.chowa[idx].aabb, &temp))
        {
            drawWsgSimple(&cg->grove.cursors[1], cg->grove.cursor.pos.x, cg->grove.cursor.pos.y);
            return;
        }
    }
    for (int idx = 0; idx < CG_GROVE_MAX_ITEMS; idx++)
    {
        // Items
        if (rectRectIntersection(rect, cg->grove.items[idx].aabb, &temp))
        {
            drawWsgSimple(&cg->grove.cursors[1], cg->grove.cursor.pos.x, cg->grove.cursor.pos.y);
            return;
        }
    }
    // Ring
    if (rectRectIntersection(rect, cg->grove.ring.aabb, &temp))
    {
        drawWsgSimple(&cg->grove.cursors[1], cg->grove.cursor.pos.x, cg->grove.cursor.pos.y);
        return;
    }
    // Otherwise
    drawWsgSimple(&cg->grove.cursors[0], cg->grove.cursor.pos.x, cg->grove.cursor.pos.y);
}

static void cg_drawItem(cGrove_t* cg, int8_t idx)
{
    if (!cg->grove.items[idx].active)
    {
        return;
    }
    int16_t xOffset = cg->grove.items[idx].aabb.pos.x - cg->grove.camera.pos.x;
    int16_t yOffset = cg->grove.items[idx].aabb.pos.y - cg->grove.camera.pos.y;
    drawWsgSimple(&cg->grove.items[idx].spr, xOffset, yOffset);
    // TODO: Shrink food
    drawText(&cg->menuFont, c555, cg->grove.items[idx].name,
             xOffset - (textWidth(&cg->menuFont, cg->grove.items[idx].name) - cg->grove.items[idx].spr.w) / 2,
             yOffset - 16);
}

static void cg_drawRing(cGrove_t* cg)
{
    int16_t xOffset = cg->grove.ring.aabb.pos.x - cg->grove.camera.pos.x;
    int16_t yOffset = cg->grove.ring.aabb.pos.y - cg->grove.camera.pos.y;
    if (cg->grove.ring.active)
    {
        drawWsgSimple(&cg->grove.itemsWSGs[11], xOffset, yOffset);
    }
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
        spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_WALK_DOWN, 0);
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
            case CHOWA_CHASE:
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
                switch (c->animFrame)
                {
                    case 0:
                    {
                        drawWsgSimple(&cg->grove.notes[0], xOffset - 7, yOffset + 9);
                        break;
                    }
                    case 1:
                    {
                        drawWsgSimple(&cg->grove.notes[1], xOffset + 9, yOffset + 15);
                        break;
                    }
                    case 2:
                    {
                        drawWsgSimple(&cg->grove.notes[2], xOffset, yOffset - 10);
                        break;
                    }
                    case 3:
                    {
                        break;
                    }
                }
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
                drawWsgSimple(spr, xOffset, yOffset);
                break;
            }
            case CHOWA_TALK:
            {
                // Update animation frame if enough time has passed
                c->frameTimer += elapsedUS;
                if (c->frameTimer > SECOND / 6)
                {
                    c->frameTimer = 0;
                    c->animFrame  = (c->animFrame + 1) % 4;
                }
                spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_WALK_SIDE, 0);
                drawWsg(spr, xOffset, yOffset, c->flip, false, 0);
                // Draw Speech bubbles. Only animate if talking to other Chowa
                if (!c->hasPartner)
                {
                    drawWsgSimple(&cg->grove.speechBubbles[0], xOffset, yOffset - 16);
                }
                else
                {
                    drawWsgSimple(&cg->grove.speechBubbles[c->animFrame], xOffset, yOffset - 16);
                }
                break;
            }
            case CHOWA_GIFT:
            {
                spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_GIFT, 0);
                drawWsgSimple(spr, xOffset, yOffset);
                break;
            }
            case CHOWA_PET:
            {
                spr = cg_getChowaWSG(cg, c->chowa, CG_ANIM_PET, 0);
                drawWsgSimple(spr, xOffset, yOffset);
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

static void cg_drawUI(cGrove_t* cg)
{
    // Draw image
    drawWsgSimple(&cg->grove.itemsWSGs[11], 15, 15);
    // Draw amount of rings
    char buffer[24];
    snprintf(buffer, sizeof(buffer) - 1, "%" PRId16, cg->grove.inv.money);
    drawText(&cg->menuFont, c555, buffer, 45, 23);
}

static void cg_drawItemsMenu(cGrove_t* cg)
{
    if (cg->grove.shopSelection == 9 || cg->grove.shopSelection == 10)
    {
        drawWsgSimpleScaled(&cg->grove.itemsWSGs[cg->grove.shopSelection],
                            (TFT_WIDTH - (cg->grove.itemsWSGs[cg->grove.shopSelection].w * 2)) / 2, 42, 2, 2);
        drawText(&cg->menuFont, c555, shopMenuItems[cg->grove.shopSelection], 16, 155);
    }
    else if (cg->grove.shopSelection == 11)
    {
        for (int idx = 0; idx < 6; idx++)
        {
            drawWsgSimpleScaled(&cg->grove.eggs[idx], 32 + (36 * idx), 80, 2, 2);
        }
        drawText(&cg->menuFont, c555, shopMenuItems[cg->grove.shopSelection], 16, 155);
    }
    else
    {
        paletteColor_t color;
        if (cg->grove.shopSelection == 7)
        {
            color = c500;
            drawWsgSimpleScaled(&cg->grove.itemsWSGs[cg->grove.shopSelection],
                                (TFT_WIDTH - (cg->grove.itemsWSGs[cg->grove.shopSelection].w * 4)) / 2, 100, 4, 4);
        }
        else
        {
            color = c555;
            drawWsgSimpleScaled(&cg->grove.itemsWSGs[cg->grove.shopSelection],
                                (TFT_WIDTH - (cg->grove.itemsWSGs[cg->grove.shopSelection].w * 4)) / 2, 60, 4, 4);
        }
        drawText(&cg->menuFont, color, shopMenuItems[cg->grove.shopSelection], 16, 155);
    }
}

static void cg_drawConfirmBox(cGrove_t* cg, char* string)
{
    // Draw box
    fillDisplayArea(32, (TFT_HEIGHT / 2) - 60, TFT_WIDTH - 32, (TFT_HEIGHT / 2) + 60, c000);

    // Draw provided text
    int16_t xOff = 40;
    int16_t yOff = (TFT_HEIGHT - 100) >> 1;
    drawTextWordWrap(&cg->menuFont, c555, string, &xOff, &yOff, TFT_WIDTH - 40, TFT_HEIGHT);

    // Draw yes/no prompt
    xOff = 40;
    yOff = (TFT_HEIGHT + 40) >> 1;
    drawTextWordWrap(&cg->menuFont, c555, confirmDefault, &xOff, &yOff, TFT_WIDTH - 40, TFT_HEIGHT);
}

static void cg_groveDebug(cGrove_t* cg)
{
    int16_t xOffset = -cg->grove.camera.pos.x;
    int16_t yOffset = -cg->grove.camera.pos.y;
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
            drawLineFast(cg->grove.chowa[i].aabb.pos.x + xOffset, cg->grove.chowa[i].aabb.pos.y + yOffset,
                         cg->grove.cursor.pos.x, cg->grove.cursor.pos.y, c505);
        }
    }
}