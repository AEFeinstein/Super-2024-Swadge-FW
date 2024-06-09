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
static void drawPinPaddle(pbPaddle_t* p);

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
    for (uint32_t wIdx = 0; wIdx < p->numWalls; wIdx++)
    {
        drawPinLine(&p->walls[wIdx]);
    }

    // Draw bumpers
    for (uint32_t uIdx = 0; uIdx < p->numBumpers; uIdx++)
    {
        drawPinCircle(&p->bumpers[uIdx]);
    }

    // Draw balls
    for (uint32_t bIdx = 0; bIdx < p->numBalls; bIdx++)
    {
        drawPinCircle(&p->balls[bIdx]);
    }

    // Draw paddles
    for (uint32_t pIdx = 0; pIdx < p->numPaddles; pIdx++)
    {
        drawPinPaddle(&p->paddles[pIdx]);
    }

    // Debug draw zones
    // for (int32_t i = 0; i < NUM_ZONES; i++)
    // {
    //     drawPinRect(&p->zones[i]);
    // }

    // Calculate and draw FPS
    int32_t startIdx  = (p->frameTimesIdx + 1) % NUM_FRAME_TIMES;
    uint32_t tElapsed = p->frameTimes[p->frameTimesIdx] - p->frameTimes[startIdx];
    if (0 != tElapsed)
    {
        uint32_t fps = (1000000 * NUM_FRAME_TIMES) / tElapsed;

        char tmp[16];
        snprintf(tmp, sizeof(tmp) - 1, "%" PRIu32, fps);
        drawText(&p->ibm_vga8, c555, tmp, 35, 2);
    }
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

/**
 * @brief Draw a pinball paddle
 *
 * @param r The paddle to draw
 */
void drawPinPaddle(pbPaddle_t* p)
{
    drawCircle(p->cPivot.pos.x, p->cPivot.pos.y, p->cPivot.radius, p->color);
    drawCircle(p->cTip.pos.x, p->cTip.pos.y, p->cTip.radius, p->color);
    drawLineFast(p->sideL.p1.x, p->sideL.p1.y, p->sideL.p2.x, p->sideL.p2.y, p->color);
    drawLineFast(p->sideR.p1.x, p->sideR.p1.y, p->sideR.p2.x, p->sideR.p2.y, p->color);
}
