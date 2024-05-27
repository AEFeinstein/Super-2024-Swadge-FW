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
static void drawPinFlipper(pbFlipper_t* f);

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

    // Draw flippers
    for (uint32_t fIdx = 0; fIdx < p->numFlippers; fIdx++)
    {
        drawPinFlipper(&p->flippers[fIdx]);
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
void drawPinCircle(pbCircle_t* circ)
{
    if (circ->filled)
    {
        drawCircleFilled((circ->c.pos.x), (circ->c.pos.y), (circ->c.radius), circ->color);
    }
    else
    {
        drawCircle((circ->c.pos.x), (circ->c.pos.y), (circ->c.radius), circ->color);
    }
}

/**
 * @brief Draw a pinball line
 *
 * @param l The line to draw
 */
void drawPinLine(pbLine_t* line)
{
    drawLineFast((line->l.p1.x), (line->l.p1.y), (line->l.p2.x), (line->l.p2.y), line->color);
}

/**
 * @brief Draw a pinball rectangle
 *
 * @param r The rectangle to draw
 */
void drawPinRect(pbRect_t* rect)
{
    drawRect(rect->r.pos.x, rect->r.pos.y, rect->r.pos.x + rect->r.width, rect->r.pos.y + rect->r.height, rect->color);
}

/**
 * @brief Draw a pinball flipper
 *
 * @param f The flipper to draw
 */
void drawPinFlipper(pbFlipper_t* f)
{
    drawCircle(f->cPivot.pos.x, f->cPivot.pos.y, f->cPivot.radius, f->color);
    drawCircle(f->cTip.pos.x, f->cTip.pos.y, f->cTip.radius, f->color);
    drawLineFast(f->sideL.p1.x, f->sideL.p1.y, f->sideL.p2.x, f->sideL.p2.y, f->color);
    drawLineFast(f->sideR.p1.x, f->sideR.p1.y, f->sideR.p2.x, f->sideR.p2.y, f->color);
}
