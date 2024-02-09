//==============================================================================
// Includes
//==============================================================================

#include "pinball_draw.h"

//==============================================================================
// Function Declarations
//==============================================================================

static void drawPinCircle(pbCircle_t* c);
static void drawPinLine(pbLine_t* l);
static void drawPinRect(pbRect_t* r);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draw a section of the background
 *
 * @param p Pinball state
 * @param x The x coordinate of the background to draw
 * @param y The y coordinate of the background to draw
 * @param w The width of the background to draw
 * @param h The height of the background to draw
 */
void pinballDrawBackground(pinball_t* p, int16_t x, int16_t y, int16_t w, int16_t h)
{
    // Fill with black
    fillDisplayArea(x, y, x + w, y + h, c000);
}

/**
 * @brief Draw the foreground
 *
 * @param p Pinball state
 */
void pinballDrawForeground(pinball_t* p)
{
    // Draw walls
    node_t* wallNode = p->walls.first;
    while (wallNode != NULL)
    {
        drawPinLine((pbLine_t*)wallNode->val);
        wallNode = wallNode->next;
    }

    // TODO draw bumpers

    // Draw balls
    node_t* ballNode = p->balls.first;
    while (ballNode != NULL)
    {
        drawPinCircle((pbCircle_t*)ballNode->val);
        ballNode = ballNode->next;
    }

    // Debug draw zones
    // for (int32_t i = 0; i < NUM_ZONES; i++)
    // {
    //     drawPinRect(&p->zones[i]);
    // }
}

/**
 * @brief Draw a pinball circle
 *
 * @param c The circle to draw
 */
void drawPinCircle(pbCircle_t* c)
{
    if (c->filled)
    {
        drawCircleFilled(FROM_FX(c->pos.x), FROM_FX(c->pos.y), FROM_FX(c->radius), c->color);
    }
    else
    {
        drawCircle(FROM_FX(c->pos.x), FROM_FX(c->pos.y), FROM_FX(c->radius), c->color);
    }
}

/**
 * @brief Draw a pinball line
 *
 * @param l The line to draw
 */
void drawPinLine(pbLine_t* l)
{
    drawLineFast(FROM_FX(l->p1.x), FROM_FX(l->p1.y), FROM_FX(l->p2.x), FROM_FX(l->p2.y), l->color);
}

/**
 * @brief Draw a pinball rectangle
 *
 * @param r The rectangle to draw
 */
void drawPinRect(pbRect_t* r)
{
    drawRect(FROM_FX(r->pos.x), FROM_FX(r->pos.y), FROM_FX(ADD_FX(r->pos.x, r->width)),
             FROM_FX(ADD_FX(r->pos.y, r->height)), r->color);
}
