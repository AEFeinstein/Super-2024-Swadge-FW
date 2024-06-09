//==============================================================================
// Includes
//==============================================================================

#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "hdw-tft.h"
#include "shapes.h"

//==============================================================================
// Defines
//==============================================================================

#define FIXEDPOINT   16
#define FIXEDPOINTD2 15

#if defined(__XTENSA__)
    /* SETUP_FOR_TURBO() is defined in hdw-tft.h, but it is overridden here because accessing dispPxL is faster than
     * getPxTftFramebuffer() */
    #undef SETUP_FOR_TURBO
    #define SETUP_FOR_TURBO() register uint32_t dispPx = (uint32_t)dispPxL;
#endif

//==============================================================================
// Function Prototypes
//==============================================================================

static void drawLineInner(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth, int xOrigin, int yOrigin,
                          int xScale, int yScale);
static void drawRectInner(int x0, int y0, int x1, int y1, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                          int yScale);
static void drawEllipseInner(int xm, int ym, int a, int b, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                             int yScale);
static void drawCircleInner(int xm, int ym, int r, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                            int yScale);
static void drawCircleFilledInner(int xm, int ym, int r, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                                  int yScale);
static void drawEllipseRectInner(int x0, int y0, int x1, int y1, paletteColor_t col, int xOrigin, int yOrigin,
                                 int xScale, int yScale);
static void drawQuadBezierSegInner(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xOrigin,
                                   int yOrigin, int xScale, int yScale);
static void drawQuadBezierInner(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xOrigin,
                                int yOrigin, int xScale, int yScale);
static void drawCubicBezierSegInner(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3,
                                    paletteColor_t col, int xOrigin, int yOrigin, int xScale, int yScale);
static void drawCubicBezierInner(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col,
                                 int xOrigin, int yOrigin, int xScale, int yScale);

//==============================================================================
// Variables
//==============================================================================

#if defined(__XTENSA__)
/* @brief Static local pointer to the framebuffer. Using this, for some reason, is faster than normal draw calls. This
 * is only set for __XTENSA__ because it jams a pointer into a 32 bit variable */
static uint32_t dispPxL = 0;
#endif

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize shape drawing by making a static local pointer to the framebuffer
 * This is done for speed reasons
 */
void initShapes(void)
{
#if defined(__XTENSA__)
    dispPxL = (uint32_t)getPxTftFramebuffer();
#endif
}

/**
 * @brief Helper function to draw a one pixel wide line that that is translated and scaled. Only a single
 * pixel is drawn for each scaled pixel, with a gap between them. To draw the rest of the pixels, this
 * function must be called with xScale and yScale adjusted for each subpixel offset within the scaled pixel.
 *
 * The line may be solid or dashed. The line coordinates are given in terms scaled pixels, not display
 * pixels. The origin of the scaled pixel coordinates can be translated using xOrigin and yOrigin,
 * essentially creating a "canvas" of scaled pixels in a section of the screen.
 *
 * @param x0 The X coordinate to start the line at, in scaled pixels
 * @param y0 The Y coordinate to start the line at, in scaled pixels
 * @param x1 The X coordinate to end the line at, in scaled pixels
 * @param y1 The Y coordinate to end the line at, in scaled pixels
 * @param col The color to draw
 * @param dashWidth The width of each dash, in scaled pixels, or 0 for a solid line
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
static void drawLineInner(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth, int xOrigin, int yOrigin,
                          int xScale, int yScale)
{
    SETUP_FOR_TURBO();
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err       = dx + dy; /* error value e_xy */
    int dashCnt   = 0;
    bool dashDraw = true;

    for (;;) /* loop */
    {
        if (dashWidth)
        {
            if (dashDraw)
            {
                TURBO_SET_PIXEL_BOUNDS(xOrigin + x0 * xScale, yOrigin + y0 * yScale, col);
            }
            dashCnt++;
            if (dashWidth == dashCnt)
            {
                dashCnt  = 0;
                dashDraw = !dashDraw;
            }
        }
        else
        {
            TURBO_SET_PIXEL_BOUNDS(xOrigin + x0 * xScale, yOrigin + y0 * yScale, col);
        }
        int e2 = 2 * err;
        if (e2 >= dy) /* e_xy+e_x > 0 */
        {
            if (x0 == x1)
            {
                break;
            }
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) /* e_xy+e_y < 0 */
        {
            if (y0 == y1)
            {
                break;
            }
            err += dx;
            y0 += sy;
        }
    }
}

/**
 * @brief Draw a one pixel wide straight line between two points.
 * The line may be solid or dashed.
 *
 * @param x0 The X coordinate to start the line at
 * @param y0 The Y coordinate to start the line at
 * @param x1 The X coordinate to end the line at
 * @param y1 The Y coordinate to end the line at
 * @param col The color to draw
 * @param dashWidth The width of each dash, or 0 for a solid line
 */
void drawLine(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth)
{
    drawLineInner(x0, y0, x1, y1, col, dashWidth, 0, 0, 1, 1);
}

/**
 * @brief Draw a line that that is translated and scaled. Scaling may make it wider than one pixel.
 * The line may be solid or dashed. The line coordinates are given in terms scaled pixels, not display
 * pixels. The origin of the scaled pixel coordinates can be translated using xOrigin and yOrigin,
 * essentially creating a "canvas" of scaled pixels in a section of the screen.
 *
 * @param x0 The X coordinate to start the line at, in scaled pixels
 * @param y0 The Y coordinate to start the line at, in scaled pixels
 * @param x1 The X coordinate to end the line at, in scaled pixels
 * @param y1 The Y coordinate to end the line at, in scaled pixels
 * @param col The color to draw
 * @param dashWidth The width of each dash, in scaled pixels, or 0 for a solid line
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
void drawLineScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int dashWidth, int xOrigin, int yOrigin,
                    int xScale, int yScale)
{
    for (uint8_t i = 0; i < xScale * yScale; i++)
    {
        drawLineInner(x0, y0, x1, y1, col, dashWidth, xOrigin + i % yScale, yOrigin + i / xScale, xScale, yScale);
    }
}

/**
 * @brief Optimized method to quickly draw a one pixel wide solid line.
 *
 * @param x0 The X coordinate to start the line at
 * @param y0 The Y coordinate to start the line at
 * @param x1 The X coordinate to end the line at
 * @param y1 The Y coordinate to end the line at
 * @param color The color to draw the line
 */
void drawLineFast(int16_t x0, int16_t y0, int16_t x1, int16_t y1, paletteColor_t color)
{
    SETUP_FOR_TURBO();
    // Tune this as a function of the size of your viewing window, line accuracy, and worst-case scenario incoming
    // lines.
    int dx            = (x1 - x0);
    int dy            = (y1 - y0);
    int sdx           = (dx > 0) ? 1 : -1;
    int sdy           = (dy > 0) ? 1 : -1;
    int yerrdiv       = (dx * sdx); // dy, but always positive.
    int xerrdiv       = (dy * sdy); // dx, but always positive.
    int yerrnumerator = 0;
    int xerrnumerator = 0;
    int cx            = x0;
    int cy            = y0;

    // Checks if both edges are outside of bounds
    // writing it this way simultaneously checks for < 0 AND >= TFT_WIDTH
    if ((uint32_t)cx >= (uint32_t)TFT_WIDTH && (uint32_t)x1 >= (uint32_t)TFT_WIDTH)
    {
        return;
    }
    if ((uint32_t)cy >= (uint32_t)TFT_HEIGHT && (uint32_t)y1 >= (uint32_t)TFT_HEIGHT)
    {
        return;
    }

    // We put the checks above to check this, in case we have a situation where
    //  we have a 0-length line outside of the viewable area.  If that happened,
    //  we would have aborted before hitting this code.

    if (yerrdiv > 0)
    {
        int dxA = 0;
        if (cx < 0)
        {
            dxA = 0 - cx;
            cx  = 0;
        }
        if (cx > (int)TFT_WIDTH - 1)
        {
            dxA = (cx - ((int)TFT_WIDTH - 1));
            cx  = (int)TFT_WIDTH - 1;
        }
        if (dxA || xerrdiv <= yerrdiv)
        {
            yerrnumerator = (((dy * sdy) << FIXEDPOINT) + yerrdiv / 2) / yerrdiv;
            if (dxA)
            {
                cy += (((yerrnumerator * dxA)) * sdy) >> FIXEDPOINT; // This "feels" right
                // Weird situation - if we cal, and now, both ends are out on the same side abort.
                if (cy < 0 && y1 < 0)
                {
                    return;
                }
                if (cy > (int)TFT_HEIGHT - 1 && y1 > (int)TFT_HEIGHT - 1)
                {
                    return;
                }
            }
        }
    }

    if (xerrdiv > 0)
    {
        int dyA = 0;
        if (cy < 0)
        {
            dyA = 0 - cy;
            cy  = 0;
        }
        if (cy > (int)TFT_HEIGHT - 1)
        {
            dyA = (cy - ((int)TFT_HEIGHT - 1));
            cy  = (int)TFT_HEIGHT - 1;
        }
        if (dyA || xerrdiv > yerrdiv)
        {
            xerrnumerator = (((dx * sdx) << FIXEDPOINT) + xerrdiv / 2) / xerrdiv;
            if (dyA)
            {
                cx += (((xerrnumerator * dyA)) * sdx) >> FIXEDPOINT; // This "feels" right.
                // If we've come to discover the line is actually out of bounds, abort.
                if (cx < 0 && x1 < 0)
                {
                    return;
                }
                if (cx > (int)TFT_WIDTH - 1 && x1 > (int)TFT_WIDTH - 1)
                {
                    return;
                }
            }
        }
    }

    if (x1 == cx && y1 == cy)
    {
        TURBO_SET_PIXEL(cx, cy, color);
        return;
    }

    // Make sure we haven't clamped the wrong way.
    // Also this checks for vertical/horizontal violations.
    if (dx > 0)
    {
        if (cx > (int)TFT_WIDTH - 1)
        {
            return;
        }
        if (cx > x1)
        {
            return;
        }
    }
    else if (dx < 0)
    {
        if (cx < 0)
        {
            return;
        }
        if (cx < x1)
        {
            return;
        }
    }

    if (dy > 0)
    {
        if (cy > (int)TFT_HEIGHT - 1)
        {
            return;
        }
        if (cy > y1)
        {
            return;
        }
    }
    else if (dy < 0)
    {
        if (cy < 0)
        {
            return;
        }
        if (cy < y1)
        {
            return;
        }
    }

    // Force clip end coordinate.
    // NOTE: We have an extra check within the inner loop, to avoid complicated math here.
    // Theoretically, we could math this so that in the end-coordinate clip stage
    // to make sure this condition just could never be hit, however, that is very
    // difficult to guarantee under all situations and may have weird edge cases.
    // So, I've decided to stick this here.

    if (xerrdiv > yerrdiv)
    {
        int xerr = 1 << FIXEDPOINTD2;
        if (x1 < 0)
        {
            x1 = 0;
        }
        if (x1 > (int)TFT_WIDTH - 1)
        {
            x1 = (int)TFT_WIDTH - 1;
        }
        x1 += sdx; // Tricky - make sure the "next" mark we hit doesn't overflow.

        if (y1 < 0)
        {
            y1 = 0;
        }
        if (y1 > (int)TFT_HEIGHT - 1)
        {
            y1 = (int)TFT_HEIGHT - 1;
        }

        for (; cy != y1; cy += sdy)
        {
            TURBO_SET_PIXEL(cx, cy, color);
            xerr += xerrnumerator;
            while (xerr >= (1 << FIXEDPOINT))
            {
                cx += sdx;
                if (cx == x1)
                {
                    return;
                }
                xerr -= 1 << FIXEDPOINT;
            }
        }
        TURBO_SET_PIXEL(cx, cy, color);
    }
    else
    {
        int yerr = 1 << FIXEDPOINTD2;

        if (y1 < 0)
        {
            y1 = 0;
        }
        if (y1 > (int)TFT_HEIGHT - 1)
        {
            y1 = (int)TFT_HEIGHT - 1;
        }
        y1 += sdy; // Tricky: Make sure the NEXT mark we hit doens't overflow.

        if (x1 < 0)
        {
            x1 = 0;
        }
        if (x1 > (int)TFT_WIDTH - 1)
        {
            x1 = (int)TFT_WIDTH - 1;
        }

        for (; cx != x1; cx += sdx)
        {
            TURBO_SET_PIXEL(cx, cy, color);
            yerr += yerrnumerator;
            while (yerr >= 1 << FIXEDPOINT)
            {
                cy += sdy;
                if (cy == y1)
                {
                    return;
                }
                yerr -= 1 << FIXEDPOINT;
            }
        }
        TURBO_SET_PIXEL(cx, cy, color);
    }
}

/**
 * @brief Helper function to draw the a one pixel wide outline of a rectangle
 *
 * @param x0 The X coordinate of the top left corner
 * @param y0 The Y coordinate of the top left corner
 * @param x1 The X coordinate of the bottom right corner
 * @param y1 The Y coordinate of the bottom right corner
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
static void drawRectInner(int x0, int y0, int x1, int y1, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                          int yScale)
{
    SETUP_FOR_TURBO();

    // Vertical lines
    for (int y = y0; y < y1; y++)
    {
        TURBO_SET_PIXEL_BOUNDS(xOrigin + x0 * xScale, yOrigin + y * yScale, col);
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (x1 - 1) * xScale, yOrigin + y * yScale, col);
    }

    // Horizontal lines
    for (int x = x0; x < x1; x++)
    {
        TURBO_SET_PIXEL_BOUNDS(xOrigin + x * xScale, yOrigin + y0 * yScale, col);
        TURBO_SET_PIXEL_BOUNDS(xOrigin + x * xScale, yOrigin + (y1 - 1) * yScale, col);
    }
}

/**
 * @brief Draw the a one pixel wide outline of a rectangle
 *
 * @param x0 The X coordinate of the top left corner
 * @param y0 The Y coordinate of the top left corner
 * @param x1 The X coordinate of the bottom right corner
 * @param y1 The Y coordinate of the bottom right corner
 * @param col The color to draw
 */
void drawRect(int x0, int y0, int x1, int y1, paletteColor_t col)
{
    drawRectInner(x0, y0, x1, y1, col, 0, 0, 1, 1);
}

/**
 * @brief Draw the outline of a rectangle that is translated and scaled. Scaling may make it wider than one pixel.
 *
 * @param x0 The X coordinate of the top left corner
 * @param y0 The Y coordinate of the top left corner
 * @param x1 The X coordinate of the bottom right corner
 * @param y1 The Y coordinate of the bottom right corner
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
void drawRectScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                    int yScale)
{
    for (uint8_t i = 0; i < xScale * yScale; i++)
    {
        drawRectInner(x0, y0, x1, y1, col, xOrigin + i % yScale, yOrigin + i / xScale, xScale, yScale);
    }
}

/**
 * @brief Optimized method to draw a triangle with outline. The interior color may be ::cTransparent to draw just an
 * outline.
 *
 * @param v0x Vertex 0's X coordinate
 * @param v0y Vertex 0's Y coordinate
 * @param v1x Vertex 1's X coordinate
 * @param v1y Vertex 1's Y coordinate
 * @param v2x Vertex 2's X coordinate
 * @param v2y Vertex 2's Y coordinate
 * @param fillColor filled area color
 * @param outlineColor outline color
 */
void drawTriangleOutlined(int16_t v0x, int16_t v0y, int16_t v1x, int16_t v1y, int16_t v2x, int16_t v2y,
                          paletteColor_t fillColor, paletteColor_t outlineColor)
{
    SETUP_FOR_TURBO();

    int16_t i16tmp;

    // Sort triangle such that v0 is the top-most vertex.
    // v0->v1 is LEFT edge.
    // v0->v2 is RIGHT edge.

    if (v0y > v1y)
    {
        i16tmp = v0x;
        v0x    = v1x;
        v1x    = i16tmp;
        i16tmp = v0y;
        v0y    = v1y;
        v1y    = i16tmp;
    }
    if (v0y > v2y)
    {
        i16tmp = v0x;
        v0x    = v2x;
        v2x    = i16tmp;
        i16tmp = v0y;
        v0y    = v2y;
        v2y    = i16tmp;
    }

    // v0 is now top-most vertex.  Now orient 2 and 3.
    // Tricky: Use slopes!  Otherwise, we could get it wrong.
    {
        int slope02;
        if (v2y - v0y)
        {
            slope02 = ((v2x - v0x) << FIXEDPOINT) / (v2y - v0y);
        }
        else
        {
            slope02 = ((v2x - v0x) > 0) ? 0x7fffff : -0x800000;
        }

        int slope01;
        if (v1y - v0y)
        {
            slope01 = ((v1x - v0x) << FIXEDPOINT) / (v1y - v0y);
        }
        else
        {
            slope01 = ((v1x - v0x) > 0) ? 0x7fffff : -0x800000;
        }

        if (slope02 < slope01)
        {
            i16tmp = v1x;
            v1x    = v2x;
            v2x    = i16tmp;
            i16tmp = v1y;
            v1y    = v2y;
            v2y    = i16tmp;
        }
    }

    // We now have a fully oriented triangle.
    int16_t x0A = v0x;
    int16_t y0A = v0y;
    int16_t x0B = v0x;
    // int16_t y0B = v0y;

    // A is to the LEFT of B.
    int dxA            = (v1x - v0x);
    int dyA            = (v1y - v0y);
    int dxB            = (v2x - v0x);
    int dyB            = (v2y - v0y);
    int sdxA           = (dxA > 0) ? 1 : -1;
    int sdyA           = (dyA > 0) ? 1 : -1;
    int sdxB           = (dxB > 0) ? 1 : -1;
    int sdyB           = (dyB > 0) ? 1 : -1;
    int xerrdivA       = (dyA * sdyA); // dx, but always positive.
    int xerrdivB       = (dyB * sdyB); // dx, but always positive.
    int xerrnumeratorA = 0;
    int xerrnumeratorB = 0;

    if (xerrdivA)
    {
        xerrnumeratorA = (((dxA * sdxA) << FIXEDPOINT) + xerrdivA / 2) / xerrdivA;
    }
    else
    {
        xerrnumeratorA = 0x7fffff;
    }

    if (xerrdivB)
    {
        xerrnumeratorB = (((dxB * sdxB) << FIXEDPOINT) + xerrdivB / 2) / xerrdivB;
    }
    else
    {
        xerrnumeratorB = 0x7fffff;
    }

    // X-clipping is handled on a per-scanline basis.
    // Y-clipping must be handled upfront.

    /*
        //Optimization BUT! Can't do this here, as we would need to be smarter about it.
        //If we do this, and the second triangle is above y=0, we'll get the wrong answer.
        if( y0A < 0 )
        {
            delta = 0 - y0A;
            y0A = 0;
            y0B = 0;
            x0A += (((xerrnumeratorA*delta)) * sdxA) >> FIXEDPOINT; //Could try rounding.
            x0B += (((xerrnumeratorB*delta)) * sdxB) >> FIXEDPOINT;
        }
    */

    {
        // Section 1 only.
        int yend = (v1y < v2y) ? v1y : v2y;
        int errA = 1 << FIXEDPOINTD2;
        int errB = 1 << FIXEDPOINTD2;
        int y;

        // Going between x0A and x0B
        for (y = y0A; y < yend; y++)
        {
            int x        = x0A;
            int endx     = x0B;
            int suppress = 1;

            if (y >= 0 && y < (int)TFT_HEIGHT)
            {
                suppress = 0;
                if (x < 0)
                {
                    x = 0;
                }
                if (endx > (int)(TFT_WIDTH))
                {
                    endx = (int)(TFT_WIDTH);
                }

                // Draw left line
                if (x0A >= 0 && x0A < (int)TFT_WIDTH)
                {
                    TURBO_SET_PIXEL(x0A, y, outlineColor);
                    x++;
                }

                // Draw body
                if (cTransparent != fillColor)
                {
                    for (; x < endx; x++)
                    {
                        TURBO_SET_PIXEL(x, y, fillColor);
                    }
                }

                // Draw right line
                if (x0B < (int)TFT_WIDTH && x0B >= 0)
                {
                    TURBO_SET_PIXEL(x0B, y, outlineColor);
                }
            }

            // Now, advance the start/end X's.
            errA += xerrnumeratorA;
            errB += xerrnumeratorB;
            while (errA >= (1 << FIXEDPOINT) && x0A != v1x)
            {
                x0A += sdxA;
                // if( x0A < 0 || x0A > (TFT_WIDTH-1) ) break;
                if (x0A >= 0 && x0A < (int)TFT_WIDTH && !suppress)
                {
                    TURBO_SET_PIXEL(x0A, y, outlineColor);
                }
                errA -= 1 << FIXEDPOINT;
            }
            while (errB >= (1 << FIXEDPOINT) && x0B != v2x)
            {
                x0B += sdxB;
                // if( x0B < 0 || x0B > (TFT_WIDTH-1) ) break;
                if (x0B >= 0 && x0B < (int)TFT_WIDTH && !suppress)
                {
                    TURBO_SET_PIXEL(x0B, y, outlineColor);
                }
                errB -= 1 << FIXEDPOINT;
            }
        }

        // We've come to the end of section 1.  Now, we need to figure

        // Now, yend is the highest possible hit on the triangle.

        // v1 is LEFT OF v2
        //  A is LEFT OF B
        if (v1y < v2y)
        {
            // V1 has terminated, move to V1->V2 but keep V0->V2[B] segment
            yend     = v2y;
            dxA      = (v2x - v1x);
            dyA      = (v2y - v1y);
            sdxA     = (dxA > 0) ? 1 : -1;
            xerrdivA = (dyA); // dx, but always positive.

            xerrnumeratorA = (((dxA * sdxA) << FIXEDPOINT) + xerrdivA / 2) / xerrdivA;

            x0A  = v1x;
            errA = 1 << FIXEDPOINTD2;
        }
        else
        {
            // V2 has terminated, move to V2->V1 but keep V0->V1[A] segment
            yend     = v1y;
            dxB      = (v1x - v2x);
            dyB      = (v1y - v2y);
            sdxB     = (dxB > 0) ? 1 : -1;
            sdyB     = (dyB > 0) ? 1 : -1;
            xerrdivB = (dyB * sdyB); // dx, but always positive.
            if (xerrdivB)
            {
                xerrnumeratorB = (((dxB * sdxB) << FIXEDPOINT) + xerrdivB / 2) / xerrdivB;
            }
            else
            {
                xerrnumeratorB = 0x7fffff;
            }
            x0B  = v2x;
            errB = 1 << FIXEDPOINTD2;
        }

        if (yend > (int)(TFT_HEIGHT - 1))
        {
            yend = (int)TFT_HEIGHT - 1;
        }

        if (xerrnumeratorA > 1000000 || xerrnumeratorB > 1000000)
        {
            if (x0A < x0B)
            {
                sdxA = 1;
                sdxB = -1;
            }
            if (x0A > x0B)
            {
                sdxA = -1;
                sdxB = 1;
            }
            if (x0A == x0B)
            {
                if (x0A >= 0 && x0A < (int)TFT_WIDTH && y >= 0 && y < (int)TFT_HEIGHT)
                {
                    TURBO_SET_PIXEL(x0A, y, outlineColor);
                }
                return;
            }
        }

        for (; y <= yend; y++)
        {
            int x        = x0A;
            int endx     = x0B;
            int suppress = 1;

            if (y >= 0 && y <= (int)(TFT_HEIGHT - 1))
            {
                suppress = 0;
                if (x < 0)
                {
                    x = 0;
                }
                if (endx >= (int)(TFT_WIDTH))
                {
                    endx = (TFT_WIDTH);
                }

                // Draw left line
                if (x0A >= 0 && x0A < (int)(TFT_WIDTH))
                {
                    TURBO_SET_PIXEL(x0A, y, outlineColor);
                    x++;
                }

                // Draw body
                if (cTransparent != fillColor)
                {
                    for (; x < endx; x++)
                    {
                        TURBO_SET_PIXEL(x, y, fillColor);
                    }
                }

                // Draw right line
                if (x0B < (int)(TFT_WIDTH) && x0B >= 0)
                {
                    TURBO_SET_PIXEL(x0B, y, outlineColor);
                }
            }

            // Now, advance the start/end X's.
            errA += xerrnumeratorA;
            errB += xerrnumeratorB;
            while (errA >= (1 << FIXEDPOINT))
            {
                x0A += sdxA;
                // if( x0A < 0 || x0A > (TFT_WIDTH-1) ) break;
                if (x0A >= 0 && x0A < (int)(TFT_WIDTH) && !suppress)
                {
                    TURBO_SET_PIXEL(x0A, y, outlineColor);
                }
                errA -= 1 << FIXEDPOINT;
                if (x0A == x0B)
                {
                    return;
                }
            }
            while (errB >= (1 << FIXEDPOINT))
            {
                x0B += sdxB;
                if (x0B >= 0 && x0B < (int)(TFT_WIDTH) && !suppress)
                {
                    TURBO_SET_PIXEL(x0B, y, outlineColor);
                }
                errB -= 1 << FIXEDPOINT;
                if (x0A == x0B)
                {
                    return;
                }
            }
        }
    }
}

/**
 * @brief Helper function to draw a one pixel wide outline of an ellipse with translation and scaling
 *
 * @param xm The X coordinate of the center of the ellipse
 * @param ym The Y coordinate of the center of the ellipse
 * @param a The X radius of the ellipse
 * @param b The Y radius of the ellipse
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
static void drawEllipseInner(int xm, int ym, int a, int b, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                             int yScale)
{
    SETUP_FOR_TURBO();

    int x = -a, y = 0;                                        /* II. quadrant from bottom left to top right */
    long e2 = (long)b * b, err = (long)x * (2 * e2 + x) + e2; /* error of 1.step */

    do
    {
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (xm - x) * xScale, yOrigin + (ym + y) * yScale, col); /*   I. Quadrant */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (xm + x) * xScale, yOrigin + (ym + y) * yScale, col); /*  II. Quadrant */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (xm + x) * xScale, yOrigin + (ym - y) * yScale, col); /* III. Quadrant */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (xm - x) * xScale, yOrigin + (ym - y) * yScale, col); /*  IV. Quadrant */
        e2 = 2 * err;
        if (e2 >= (x * 2 + 1) * (long)b * b) /* e_xy+e_x > 0 */
        {
            err += (++x * 2 + 1) * (long)b * b;
        }
        if (e2 <= (y * 2 + 1) * (long)a * a) /* e_xy+e_y < 0 */
        {
            err += (++y * 2 + 1) * (long)a * a;
        }
    } while (x <= 0);

    while (y++ < b) /* too early stop of flat ellipses a=1, */
    {
        TURBO_SET_PIXEL_BOUNDS(xOrigin + xm * xScale, yOrigin + (ym + y) * yScale, col); /* -> finish tip of ellipse */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + xm * xScale, yOrigin + (ym - y) * yScale, col);
    }
}

/**
 * @brief Draw the outline of an ellipse with translation and scaling. Scaling may make it wider than one pixel.
 *
 * @param xm The X coordinate of the center of the ellipse
 * @param ym The Y coordinate of the center of the ellipse
 * @param a The X radius of the ellipse
 * @param b The Y radius of the ellipse
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
void drawEllipseScaled(int xm, int ym, int a, int b, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                       int yScale)
{
    for (uint8_t i = 0; i < xScale * yScale; i++)
    {
        drawEllipseInner(xm, ym, a, b, col, xOrigin + i % yScale, yOrigin + i / xScale, xScale, yScale);
    }
}

/**
 * @brief Draw a one pixel wide outline of an ellipse
 *
 * @param xm The X coordinate of the center of the ellipse
 * @param ym The Y coordinate of the center of the ellipse
 * @param a The X radius of the ellipse
 * @param b The Y radius of the ellipse
 * @param col The color to draw
 */
void drawEllipse(int xm, int ym, int a, int b, paletteColor_t col)
{
    SETUP_FOR_TURBO();

    long x = -a, y = 0;                      /* II. quadrant from bottom left to top right */
    long e2 = b, dx = (1 + 2 * x) * e2 * e2; /* error increment  */
    long dy = x * x, err = dx + dy;          /* error of 1.step */

    do
    {
        TURBO_SET_PIXEL_BOUNDS(xm - x, ym + y, col); /*   I. Quadrant */
        TURBO_SET_PIXEL_BOUNDS(xm + x, ym + y, col); /*  II. Quadrant */
        TURBO_SET_PIXEL_BOUNDS(xm + x, ym - y, col); /* III. Quadrant */
        TURBO_SET_PIXEL_BOUNDS(xm - x, ym - y, col); /*  IV. Quadrant */
        e2 = 2 * err;
        if (e2 >= dx)
        {
            x++;
            err += dx += 2 * (long)b * b;
        } /* x step */
        if (e2 <= dy)
        {
            y++;
            err += dy += 2 * (long)a * a;
        } /* y step */
    } while (x <= 0);

    while (y++ < b) /* too early stop for flat ellipses with a=1, */
    {
        TURBO_SET_PIXEL_BOUNDS(xm, ym + y, col); /* -> finish tip of ellipse */
        TURBO_SET_PIXEL_BOUNDS(xm, ym - y, col);
    }
}

/**
 * @brief Helper function to draw the outline of a circle with translation and scaling
 *
 * @param xm The X coordinate of the center of the circle
 * @param ym The Y coordinate of the center of the circle
 * @param r The radius of the circle
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
static void drawCircleInner(int xm, int ym, int r, paletteColor_t col, int xOrigin, int yOrigin, int xScale, int yScale)
{
    SETUP_FOR_TURBO();

    int x = -r, y = 0, err = 2 - 2 * r; /* bottom left to top right */
    do
    {
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (xm - x) * xScale, yOrigin + (ym + y) * yScale, col); /*   I. Quadrant +x +y */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (xm - y) * xScale, yOrigin + (ym - x) * yScale, col); /*  II. Quadrant -x +y */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (xm + x) * xScale, yOrigin + (ym - y) * yScale, col); /* III. Quadrant -x -y */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (xm + y) * xScale, yOrigin + (ym + x) * yScale, col); /*  IV. Quadrant +x -y */
        r = err;
        if (r <= y)
        {
            err += ++y * 2 + 1; /* e_xy+e_y < 0 */
        }
        if (r > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
        {
            err += ++x * 2 + 1; /* -> x-step now */
        }
    } while (x < 0);
}

/**
 * @brief Draw the one pixel wide outline of a circle
 *
 * @param xm The X coordinate of the center of the circle
 * @param ym The Y coordinate of the center of the circle
 * @param r The radius of the circle
 * @param col The color to draw
 */
void drawCircle(int xm, int ym, int r, paletteColor_t col)
{
    drawCircleInner(xm, ym, r, col, 0, 0, 1, 1);
}

/**
 * @brief Draw the outline of a circle with translation and scaling. Scaling may make it wider than one pixel.
 *
 * @param xm The X coordinate of the center of the circle
 * @param ym The Y coordinate of the center of the circle
 * @param r The radius of the circle
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
void drawCircleScaled(int xm, int ym, int r, paletteColor_t col, int xOrigin, int yOrigin, int xScale, int yScale)
{
    for (uint8_t i = 0; i < xScale * yScale; i++)
    {
        drawCircleInner(xm, ym, r, col, xOrigin + i % yScale, yOrigin + i / xScale, xScale, yScale);
    }
}

/**
 * @brief Draw the one pixel wide outline of the quadrants of a circle
 *
 * @param xm The X coordinate of the center of the circle
 * @param ym The Y coordinate of the center of the circle
 * @param r The radius of the circle
 * @param q1 True to draw the top left quadrant
 * @param q2 True to draw the top right quadrant
 * @param q3 True to draw the bottom right quadrant
 * @param q4 True to draw the bottom left quadrant
 * @param col The color to draw
 */
void drawCircleQuadrants(int xm, int ym, int r, bool q1, bool q2, bool q3, bool q4, paletteColor_t col)
{
    SETUP_FOR_TURBO();

    int x = -r, y = 0, err = 2 - 2 * r; /* bottom left to top right */
    do
    {
        if (q1)
        {
            TURBO_SET_PIXEL_BOUNDS(xm - x, ym + y, col); /*   I. Quadrant +x +y */
        }
        if (q2)
        {
            TURBO_SET_PIXEL_BOUNDS(xm - y, ym - x, col); /*  II. Quadrant -x +y */
        }
        if (q3)
        {
            TURBO_SET_PIXEL_BOUNDS(xm + x, ym - y, col); /* III. Quadrant -x -y */
        }
        if (q4)
        {
            TURBO_SET_PIXEL_BOUNDS(xm + y, ym + x, col); /*  IV. Quadrant +x -y */
        }
        r = err;
        if (r <= y)
        {
            err += ++y * 2 + 1; /* e_xy+e_y < 0 */
        }
        if (r > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
        {
            err += ++x * 2 + 1; /* -> x-step now */
        }
    } while (x < 0);
}

/**
 * @brief Helper function to draw a filled circle with translation and scaling
 *
 * @param xm The X coordinate of the center of the circle
 * @param ym The Y coordinate of the center of the circle
 * @param r The radius of the circle
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
static void drawCircleFilledInner(int xm, int ym, int r, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                                  int yScale)
{
    SETUP_FOR_TURBO();

    int x = -r, y = 0, err = 2 - 2 * r; /* bottom left to top right */
    do
    {
        for (int lineX = xm + x; lineX <= xm - x; lineX++)
        {
            TURBO_SET_PIXEL_BOUNDS(xOrigin + lineX * xScale, yOrigin + (ym - y) * yScale, col);
            TURBO_SET_PIXEL_BOUNDS(xOrigin + lineX * xScale, yOrigin + (ym + y) * yScale, col);
        }

        r = err;
        if (r <= y)
        {
            err += ++y * 2 + 1; /* e_xy+e_y < 0 */
        }
        if (r > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
        {
            err += ++x * 2 + 1; /* -> x-step now */
        }
    } while (x < 0);
}

/**
 * @brief Draw the outline of a circle with the given stroke width
 *
 * @param xm The X coordinate of the center of the circle
 * @param ym The Y coordinate of the center of the circle
 * @param r The radius of the circle
 * @param stroke The stroke width (pixels inside the radius)
 * @param col The color to draw
 */
void drawCircleOutline(int xm, int ym, int r, int stroke, paletteColor_t col)
{
    SETUP_FOR_TURBO();

    // Outer circle
    int x = -r, y = 0, err = 2 - 2 * r; /* bottom left to top right */

    // Inner circle
    int r_inner = r - stroke;
    int x_inner = -r_inner, y_inner = 0, err_inner = 2 - 2 * r_inner; /* bottom left to top right */

    // Iterates over Y
    do
    {
        // Iterates over X
        for (int lineX = xm + x; lineX <= xm - x; lineX++)
        {
            // Only draw the outline
            if (lineX < (xm + x_inner) || lineX > (xm - x_inner))
            {
                TURBO_SET_PIXEL_BOUNDS(lineX, (ym - y), col);
                TURBO_SET_PIXEL_BOUNDS(lineX, (ym + y), col);
            }
        }

        // Iterate the outer circle
        r = err;
        if (r <= y)
        {
            err += ++y * 2 + 1; /* e_xy+e_y < 0 */
        }
        if (r > x || err > y) /* e_xy+e_x > 0 or no 2nd y-step */
        {
            err += ++x * 2 + 1; /* -> x-step now */
        }

        // Iterate the inner circle to match
        while (y_inner != y)
        {
            r_inner = err_inner;
            if (r_inner <= y_inner)
            {
                err_inner += ++y_inner * 2 + 1; /* e_xy+e_y < 0 */
            }
            if (r_inner > x_inner || err_inner > y_inner) /* e_xy+e_x > 0 or no 2nd y-step */
            {
                err_inner += ++x_inner * 2 + 1; /* -> x-step now */
            }
        }
    } while (x < 0);
}

/**
 * @brief Draw a filled circle
 *
 * @param xm The X coordinate of the center of the circle
 * @param ym The Y coordinate of the center of the circle
 * @param r The radius of the circle
 * @param col The color to draw
 */
void drawCircleFilled(int xm, int ym, int r, paletteColor_t col)
{
    drawCircleFilledInner(xm, ym, r, col, 0, 0, 1, 1);
}

/**
 * @brief Draw a filed circle with translation and scaling
 *
 * @param xm The X coordinate of the center of the circle
 * @param ym The Y coordinate of the center of the circle
 * @param r The radius of the circle
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
void drawCircleFilledScaled(int xm, int ym, int r, paletteColor_t col, int xOrigin, int yOrigin, int xScale, int yScale)
{
    for (uint8_t i = 0; i < xScale * yScale; i++)
    {
        drawCircleFilledInner(xm, ym, r, col, xOrigin + i % yScale, yOrigin + i / xScale, xScale, yScale);
    }
}

/**
 * @brief Helper function to draw a one pixel wide outline of an ellipse with translation and scaling within a rectangle
 *
 * @param x0 The X coordinate of the top left corner
 * @param y0 The Y coordinate of the top left corner
 * @param x1 The X coordinate of the bottom right corner
 * @param y1 The Y coordinate of the bottom right corner
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
static void drawEllipseRectInner(int x0, int y0, int x1, int y1, paletteColor_t col, int xOrigin, int yOrigin,
                                 int xScale, int yScale) /* rectangular parameter enclosing the ellipse */
{
    SETUP_FOR_TURBO();

    long a = abs(x1 - x0), b = abs(y1 - y0), b1 = b & 1;          /* diameter */
    double dx = 4 * (1.0 - a) * b * b, dy = 4 * (b1 + 1) * a * a; /* error increment */
    double err = dx + dy + b1 * a * a;                            /* error of 1.step */

    if (x0 > x1)
    {
        x0 = x1;
        x1 += a;
    } /* if called with swapped points */
    if (y0 > y1)
    {
        y0 = y1; /* .. exchange them */
    }
    y0 += (b + 1) / 2;
    y1 = y0 - b1; /* starting pixel */
    a  = 8 * a * a;
    b1 = 8 * b * b;

    do
    {
        TURBO_SET_PIXEL_BOUNDS(xOrigin + x1 * xScale, yOrigin + y0 * yScale, col); /*   I. Quadrant */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + x0 * xScale, yOrigin + y0 * yScale, col); /*  II. Quadrant */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + x0 * xScale, yOrigin + y1 * yScale, col); /* III. Quadrant */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + x1 * xScale, yOrigin + y1 * yScale, col); /*  IV. Quadrant */
        double e2 = 2 * err;
        if (e2 <= dy)
        {
            y0++;
            y1--;
            err += dy += a;
        } /* y step */
        if (e2 >= dx || 2 * err > dy)
        {
            x0++;
            x1--;
            err += dx += b1;
        } /* x step */
    } while (x0 <= x1);

    while (y0 - y1 <= b) /* too early stop of flat ellipses a=1 */
    {
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (x0 - 1) * xScale, yOrigin + y0 * yScale, col); /* -> finish tip of ellipse */
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (x1 + 1) * xScale, yOrigin + y0++ * yScale, col);
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (x0 - 1) * xScale, yOrigin + y1 * yScale, col);
        TURBO_SET_PIXEL_BOUNDS(xOrigin + (x1 + 1) * xScale, yOrigin + y1-- * yScale, col);
    }
}

/**
 * @brief Draw a one pixel wide outline of an ellipse within a rectangle
 *
 * @param x0 The X coordinate of the top left corner
 * @param y0 The Y coordinate of the top left corner
 * @param x1 The X coordinate of the bottom right corner
 * @param y1 The Y coordinate of the bottom right corner
 * @param col The color to draw
 */
void drawEllipseRect(int x0, int y0, int x1, int y1,
                     paletteColor_t col) /* rectangular parameter enclosing the ellipse */
{
    drawEllipseRectInner(x0, y0, x1, y1, col, 0, 0, 1, 1);
}

/**
 * @brief Draw the outline of an ellipse with translation and scaling within a rectangle. Scaling may make it wider than
 * one pixel.
 *
 * @param x0 The X coordinate of the top left corner
 * @param y0 The Y coordinate of the top left corner
 * @param x1 The X coordinate of the bottom right corner
 * @param y1 The Y coordinate of the bottom right corner
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
void drawEllipseRectScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xOrigin, int yOrigin, int xScale,
                           int yScale)
{
    for (uint8_t i = 0; i < xScale * yScale; i++)
    {
        drawEllipseRectInner(x0, y0, x1, y1, col, xOrigin + i % yScale, yOrigin + i / xScale, xScale, yScale);
    }
}

/**
 * @brief Helper function to draw a one pixel wide quadratic Bezier curve with translation and scaling.
 * This is limited to gradients without sign change.
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
static void drawQuadBezierSegInner(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xOrigin,
                                   int yOrigin, int xScale, int yScale)
{
    SETUP_FOR_TURBO();

    int sx = x2 - x1, sy = y2 - y1;
    long xx = x0 - x1, yy = y0 - y1; /* relative values for checks */
    double cur = xx * sy - yy * sx;  /* curvature */

    assert(xx * sx <= 0 && yy * sy <= 0); /* sign of gradient must not change */

    if (sx * (long)sx + sy * (long)sy > xx * xx + yy * yy) /* begin with longer part */
    {
        x2  = x0;
        x0  = sx + x1;
        y2  = y0;
        y0  = sy + y1;
        cur = -cur; /* swap P0 P2 */
    }
    if (cur != 0) /* no straight line */
    {
        xx += sx;
        xx *= sx = x0 < x2 ? 1 : -1; /* x step direction */
        yy += sy;
        yy *= sy = y0 < y2 ? 1 : -1; /* y step direction */
        long xy  = 2 * xx * yy;
        xx *= xx;
        yy *= yy;              /* differences 2nd degree */
        if (cur * sx * sy < 0) /* negated curvature? */
        {
            xx  = -xx;
            yy  = -yy;
            xy  = -xy;
            cur = -cur;
        }
        double dx = 4.0 * sy * cur * (x1 - x0) + xx - xy; /* differences 1st degree */
        double dy = 4.0 * sx * cur * (y0 - y1) + yy - xy;
        xx += xx;
        yy += yy;
        double err = dx + dy + xy; /* error 1st step */
        do
        {
            TURBO_SET_PIXEL_BOUNDS(xOrigin + x0 * xScale, yOrigin + y0 * yScale, col); /* draw curve */
            if (x0 == x2 && y0 == y2)
            {
                return; /* last pixel -> curve finished */
            }
            y1 = 2 * err < dx; /* save value for test of y step */
            if (2 * err > dy)
            {
                x0 += sx;
                dx -= xy;
                err += dy += yy;
            } /* x step */
            if (y1)
            {
                y0 += sy;
                dy -= xy;
                err += dx += xx;
            }                       /* y step */
        } while (dy < 0 && dx > 0); /* gradient negates -> algorithm fails */
    }
    drawLineScaled(x0, y0, x2, y2, col, 0, xOrigin, yOrigin, xScale, yScale); /* draw remaining part to end */
}

/**
 * @brief Draw a one pixel wide quadratic Bezier curve.
 * This is limited to gradients without sign change.
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param col The color to draw
 */
void drawQuadBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col)
{
    drawQuadBezierSegInner(x0, y0, x1, y1, x2, y2, col, 0, 0, 1, 1);
}

/**
 * @brief Draw a one pixel wide quadratic Bezier curve with translation and scaling.
 * This is limited to gradients without sign change.
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
void drawQuadBezierSegScaled(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xOrigin,
                             int yOrigin, int xScale, int yScale)
{
    for (uint8_t i = 0; i < xScale * yScale; i++)
    {
        drawQuadBezierSegInner(x0, y0, x1, y1, x2, y2, col, xOrigin + i % yScale, yOrigin + i / xScale, xScale, yScale);
    }
}
/**
 * @brief Helper function to draw a one pixel wide quadratic Bezier curve with translation and scaling
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
static void drawQuadBezierInner(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xOrigin,
                                int yOrigin, int xScale, int yScale)
{
    int x = x0 - x1, y = y0 - y1;
    double t = x0 - 2 * x1 + x2, r;

    if ((long)x * (x2 - x1) > 0) /* horizontal cut at P4? */
    {
        if ((long)y * (y2 - y1) > 0)                       /* vertical cut at P6 too? */
            if (fabs((y0 - 2 * y1 + y2) / t * x) > abs(y)) /* which first? */
            {
                x0 = x2;
                x2 = x + x1;
                y0 = y2;
                y2 = y + y1; /* swap points */
            }                /* now horizontal cut at P4 comes first */
        t = (x0 - x1) / t;
        r = (1 - t) * ((1 - t) * y0 + 2.0 * t * y1) + t * t * y2; /* By(t=P4) */
        t = (x0 * x2 - x1 * x1) * t / (x0 - x1);                  /* gradient dP4/dx=0 */
        x = floor(t + 0.5);
        y = floor(r + 0.5);
        r = (y1 - y0) * (t - x0) / (x1 - x0) + y0; /* intersect P3 | P0 P1 */
        drawQuadBezierSegInner(x0, y0, x, floor(r + 0.5), x, y, col, xOrigin, yOrigin, xScale, yScale);
        r  = (y1 - y2) * (t - x2) / (x1 - x2) + y2; /* intersect P4 | P1 P2 */
        x0 = x1 = x;
        y0      = y;
        y1      = floor(r + 0.5); /* P0 = P4, P1 = P8 */
    }
    if ((long)(y0 - y1) * (y2 - y1) > 0) /* vertical cut at P6? */
    {
        t = y0 - 2 * y1 + y2;
        t = (y0 - y1) / t;
        r = (1 - t) * ((1 - t) * x0 + 2.0 * t * x1) + t * t * x2; /* Bx(t=P6) */
        t = (y0 * y2 - y1 * y1) * t / (y0 - y1);                  /* gradient dP6/dy=0 */
        x = floor(r + 0.5);
        y = floor(t + 0.5);
        r = (x1 - x0) * (t - y0) / (y1 - y0) + x0; /* intersect P6 | P0 P1 */
        drawQuadBezierSegInner(x0, y0, floor(r + 0.5), y, x, y, col, xOrigin, yOrigin, xScale, yScale);
        r  = (x1 - x2) * (t - y2) / (y1 - y2) + x2; /* intersect P7 | P1 P2 */
        x0 = x;
        x1 = floor(r + 0.5);
        y0 = y1 = y; /* P0 = P6, P1 = P7 */
    }
    drawQuadBezierSegInner(x0, y0, x1, y1, x2, y2, col, xOrigin, yOrigin, xScale, yScale); /* remaining part */
}
/**
 * @brief Draw a one pixel wide quadratic Bezier curve
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param col The color to draw
 */
void drawQuadBezier(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col)
{
    drawQuadBezierInner(x0, y0, x1, y1, x2, y2, col, 0, 0, 1, 1);
}

/**
 * @brief Draw a one pixel wide quadratic Bezier curve with translation and scaling
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
void drawQuadBezierScaled(int x0, int y0, int x1, int y1, int x2, int y2, paletteColor_t col, int xOrigin, int yOrigin,
                          int xScale, int yScale)
{
    for (uint8_t i = 0; i < xScale * yScale; i++)
    {
        drawQuadBezierInner(x0, y0, x1, y1, x2, y2, col, xOrigin + i % yScale, yOrigin + i / xScale, xScale, yScale);
    }
}

/**
 * @brief Draw a limited one pixel wide rational Bezier curve with squared weight
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param w Squared Weight
 * @param col The color to draw
 */
void drawQuadRationalBezierSeg(int x0, int y0, int x1, int y1, int x2, int y2, float w, paletteColor_t col)
{
    SETUP_FOR_TURBO();

    int sx = x2 - x1, sy = y2 - y1; /* relative values for checks */
    double dx = x0 - x2, dy = y0 - y2, xx = x0 - x1, yy = y0 - y1;
    double xy = xx * sy + yy * sx, cur = xx * sy - yy * sx; /* curvature */

    assert(xx * sx <= 0.0 && yy * sy <= 0.0); /* sign of gradient must not change */

    if (cur != 0.0 && w > 0.0) /* no straight line */
    {
        if (sx * (long)sx + sy * (long)sy > xx * xx + yy * yy) /* begin with longer part */
        {
            x2 = x0;
            x0 -= dx;
            y2 = y0;
            y0 -= dy;
            cur = -cur; /* swap P0 P2 */
        }
        xx = 2.0 * (4.0 * w * sx * xx + dx * dx); /* differences 2nd degree */
        yy = 2.0 * (4.0 * w * sy * yy + dy * dy);
        sx = x0 < x2 ? 1 : -1; /* x step direction */
        sy = y0 < y2 ? 1 : -1; /* y step direction */
        xy = -2.0 * sx * sy * (2.0 * w * xy + dx * dy);

        if (cur * sx * sy < 0.0) /* negated curvature? */
        {
            xx  = -xx;
            yy  = -yy;
            xy  = -xy;
            cur = -cur;
        }
        dx = 4.0 * w * (x1 - x0) * sy * cur + xx / 2.0 + xy; /* differences 1st degree */
        dy = 4.0 * w * (y0 - y1) * sx * cur + yy / 2.0 + xy;

        if (w < 0.5 && (dy > xy || dx < xy)) /* flat ellipse, algorithm fails */
        {
            cur = (w + 1.0) / 2.0;
            w   = sqrt(w);
            xy  = 1.0 / (w + 1.0);
            sx  = floor((x0 + 2.0 * w * x1 + x2) * xy / 2.0 + 0.5); /* subdivide curve in half */
            sy  = floor((y0 + 2.0 * w * y1 + y2) * xy / 2.0 + 0.5);
            dx  = floor((w * x1 + x0) * xy + 0.5);
            dy  = floor((y1 * w + y0) * xy + 0.5);
            drawQuadRationalBezierSeg(x0, y0, dx, dy, sx, sy, cur, col); /* draw separately */
            dx = floor((w * x1 + x2) * xy + 0.5);
            dy = floor((y1 * w + y2) * xy + 0.5);
            drawQuadRationalBezierSeg(sx, sy, dx, dy, x2, y2, cur, col);
            return;
        }
        double err = dx + dy - xy; /* error 1.step */
        do
        {
            TURBO_SET_PIXEL_BOUNDS(x0, y0, col); /* draw curve */
            if (x0 == x2 && y0 == y2)
            {
                return; /* last pixel -> curve finished */
            }
            x1 = 2 * err > dy;
            y1 = 2 * (err + yy) < -dy; /* save value for test of x step */
            if (2 * err < dx || y1)
            {
                y0 += sy;
                dy += xy;
                err += dx += xx;
            } /* y step */
            if (2 * err > dx || x1)
            {
                x0 += sx;
                dx += xy;
                err += dy += yy;
            }                           /* x step */
        } while (dy <= xy && dx >= xy); /* gradient negates -> algorithm fails */
    }
    drawLine(x0, y0, x2, y2, col, 0); /* draw remaining needle to end */
}

/**
 * @brief Draw any one pixel wide quadratic rational Bezier curve with squared weight
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param w Squared Weight
 * @param col The color to draw
 */
void drawQuadRationalBezier(int x0, int y0, int x1, int y1, int x2, int y2, float w, paletteColor_t col)
{
    int x = x0 - 2 * x1 + x2, y = y0 - 2 * y1 + y2;
    double xx = x0 - x1, yy = y0 - y1, ww, t, q;

    assert(w >= 0.0);

    if (xx * (x2 - x1) > 0) /* horizontal cut at P4? */
    {
        if (yy * (y2 - y1) > 0)              /* vertical cut at P6 too? */
            if (fabs(xx * y) > fabs(yy * x)) /* which first? */
            {
                x0 = x2;
                x2 = xx + x1;
                y0 = y2;
                y2 = yy + y1; /* swap points */
            }                 /* now horizontal cut at P4 comes first */
        if (x0 == x2 || w == 1.0)
        {
            t = (x0 - x1) / (double)x;
        }
        else /* non-rational or rational case */
        {
            q = sqrt(4.0 * w * w * (x0 - x1) * (x2 - x1) + (x2 - x0) * (long)(x2 - x0));
            if (x1 < x0)
            {
                q = -q;
            }
            t = (2.0 * w * (x0 - x1) - x0 + x2 + q) / (2.0 * (1.0 - w) * (x2 - x0)); /* t at P4 */
        }
        q  = 1.0 / (2.0 * t * (1.0 - t) * (w - 1.0) + 1.0);                         /* sub-divide at t */
        xx = (t * t * (x0 - 2.0 * w * x1 + x2) + 2.0 * t * (w * x1 - x0) + x0) * q; /* = P4 */
        yy = (t * t * (y0 - 2.0 * w * y1 + y2) + 2.0 * t * (w * y1 - y0) + y0) * q;
        ww = t * (w - 1.0) + 1.0;
        ww *= ww * q;                                 /* squared weight P3 */
        w  = ((1.0 - t) * (w - 1.0) + 1.0) * sqrt(q); /* weight P8 */
        x  = floor(xx + 0.5);
        y  = floor(yy + 0.5);                        /* P4 */
        yy = (xx - x0) * (y1 - y0) / (x1 - x0) + y0; /* intersect P3 | P0 P1 */
        drawQuadRationalBezierSeg(x0, y0, x, floor(yy + 0.5), x, y, ww, col);
        yy = (xx - x2) * (y1 - y2) / (x1 - x2) + y2; /* intersect P4 | P1 P2 */
        y1 = floor(yy + 0.5);
        x0 = x1 = x;
        y0      = y; /* P0 = P4, P1 = P8 */
    }
    if ((y0 - y1) * (long)(y2 - y1) > 0) /* vertical cut at P6? */
    {
        if (y0 == y2 || w == 1.0)
        {
            t = (y0 - y1) / (y0 - 2.0 * y1 + y2);
        }
        else /* non-rational or rational case */
        {
            q = sqrt(4.0 * w * w * (y0 - y1) * (y2 - y1) + (y2 - y0) * (long)(y2 - y0));
            if (y1 < y0)
            {
                q = -q;
            }
            t = (2.0 * w * (y0 - y1) - y0 + y2 + q) / (2.0 * (1.0 - w) * (y2 - y0)); /* t at P6 */
        }
        q  = 1.0 / (2.0 * t * (1.0 - t) * (w - 1.0) + 1.0);                         /* sub-divide at t */
        xx = (t * t * (x0 - 2.0 * w * x1 + x2) + 2.0 * t * (w * x1 - x0) + x0) * q; /* = P6 */
        yy = (t * t * (y0 - 2.0 * w * y1 + y2) + 2.0 * t * (w * y1 - y0) + y0) * q;
        ww = t * (w - 1.0) + 1.0;
        ww *= ww * q;                                 /* squared weight P5 */
        w  = ((1.0 - t) * (w - 1.0) + 1.0) * sqrt(q); /* weight P7 */
        x  = floor(xx + 0.5);
        y  = floor(yy + 0.5);                        /* P6 */
        xx = (x1 - x0) * (yy - y0) / (y1 - y0) + x0; /* intersect P6 | P0 P1 */
        drawQuadRationalBezierSeg(x0, y0, floor(xx + 0.5), y, x, y, ww, col);
        xx = (x1 - x2) * (yy - y2) / (y1 - y2) + x2; /* intersect P7 | P1 P2 */
        x1 = floor(xx + 0.5);
        x0 = x;
        y0 = y1 = y; /* P0 = P6, P1 = P7 */
    }
    drawQuadRationalBezierSeg(x0, y0, x1, y1, x2, y2, w * w, col); /* remaining */
}

/**
 * @brief Draw an ellipse rotated by angle (radian)
 *
 * @param x The X coordinate of the center of the ellipse
 * @param y The Y coordinate of the center of the ellipse
 * @param a The X radius of the ellipse
 * @param b The Y radius of the ellipse
 * @param angle The angle, in radians, to rotate the ellipse by
 * @param col The color to draw
 */
void drawRotatedEllipse(int x, int y, int a, int b, float angle, paletteColor_t col)
{
    float xd = (long)a * a, yd = (long)b * b;
    float s = sin(angle), zd = (xd - yd) * s;       /* ellipse rotation */
    xd = sqrt(xd - zd * s), yd = sqrt(yd + zd * s); /* surrounding rectangle */
    a  = xd + 0.5;
    b  = yd + 0.5;
    zd = zd * a * b / (xd * yd); /* scale to integer */
    drawRotatedEllipseRect(x - a, y - b, x + a, y + b, (long)(4 * zd * cos(angle)), col);
}

/**
 * @brief Draw an ellipse, bounded by a rectangle, rotated (integer)
 *
 * @param x0 The X coordinate of the top left corner
 * @param y0 The Y coordinate of the top left corner
 * @param x1 The X coordinate of the bottom right corner
 * @param y1 The Y coordinate of the bottom right corner
 * @param zd The integer angle to rotate the ellipse
 * @param col The color to draw
 */
void drawRotatedEllipseRect(int x0, int y0, int x1, int y1, long zd, paletteColor_t col)
{
    int xd = x1 - x0, yd = y1 - y0;
    float w = xd * (long)yd;
    if (zd == 0)
    {
        return drawEllipseRect(x0, y0, x1, y1, col); /* looks nicer */
    }
    if (w != 0.0)
    {
        w = (w - zd) / (w + w); /* squared weight of P1 */
    }
    assert(w <= 1.0 && w >= 0.0); /* limit angle to |zd|<=xd*yd */
    xd = floor(xd * w + 0.5);
    yd = floor(yd * w + 0.5); /* snap xe,ye to int */
    drawQuadRationalBezierSeg(x0, y0 + yd, x0, y0, x0 + xd, y0, 1.0 - w, col);
    drawQuadRationalBezierSeg(x0, y0 + yd, x0, y1, x1 - xd, y1, w, col);
    drawQuadRationalBezierSeg(x1, y1 - yd, x1, y1, x1 - xd, y1, 1.0 - w, col);
    drawQuadRationalBezierSeg(x1, y1 - yd, x1, y0, x0 + xd, y0, w, col);
}

/**
 * @brief Helper function to draw a limited cubic Bezier segment
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param x3 The X coordinate of the fourth point
 * @param y3 The Y coordinate of the fourth point
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
static void drawCubicBezierSegInner(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3,
                                    paletteColor_t col, int xOrigin, int yOrigin, int xScale, int yScale)
{
    SETUP_FOR_TURBO();

    int f, fx, fy, leg = 1;
    int sx = x0 < x3 ? 1 : -1, sy = y0 < y3 ? 1 : -1; /* step direction */
    float xc = -fabs(x0 + x1 - x2 - x3), xa = xc - 4 * sx * (x1 - x2), xb = sx * (x0 - x1 - x2 + x3);
    float yc = -fabs(y0 + y1 - y2 - y3), ya = yc - 4 * sy * (y1 - y2), yb = sy * (y0 - y1 - y2 + y3);
    double ab, ac, bc, cb, xx, xy, yy, dx, dy, ex, *pxy, EP = 0.01;
    /* check for curve restrains */
    /* slope P0-P1 == P2-P3    and  (P0-P3 == P1-P2      or   no slope change) */
    assert((x1 - x0) * (x2 - x3) < EP && ((x3 - x0) * (x1 - x2) < EP || xb * xb < xa * xc + EP));
    assert((y1 - y0) * (y2 - y3) < EP && ((y3 - y0) * (y1 - y2) < EP || yb * yb < ya * yc + EP));

    if (xa == 0 && ya == 0) /* quadratic Bezier */
    {
        sx = floor((3 * x1 - x0 + 1) / 2);
        sy = floor((3 * y1 - y0 + 1) / 2); /* new midpoint */
        return drawQuadBezierSegInner(x0, y0, sx, sy, x3, y3, col, xOrigin, yOrigin, xScale, yScale);
    }
    x1 = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0) + 1; /* line lengths */
    x2 = (x2 - x3) * (x2 - x3) + (y2 - y3) * (y2 - y3) + 1;
    do /* loop over both ends */
    {
        ab = xa * yb - xb * ya;
        ac = xa * yc - xc * ya;
        bc = xb * yc - xc * yb;
        ex = ab * (ab + ac - 3 * bc) + ac * ac; /* P0 part of self-intersection loop? */
        f  = ex > 0 ? 1 : sqrt(1 + 1024 / x1);  /* calculate resolution */
        ab *= f;
        ac *= f;
        bc *= f;
        ex *= f * f; /* increase resolution */
        xy = 9 * (ab + ac + bc) / 8;
        cb = 8 * (xa - ya); /* init differences of 1st degree */
        dx = 27 * (8 * ab * (yb * yb - ya * yc) + ex * (ya + 2 * yb + yc)) / 64 - ya * ya * (xy - ya);
        dy = 27 * (8 * ab * (xb * xb - xa * xc) - ex * (xa + 2 * xb + xc)) / 64 - xa * xa * (xy + xa);
        /* init differences of 2nd degree */
        xx = 3 * (3 * ab * (3 * yb * yb - ya * ya - 2 * ya * yc) - ya * (3 * ac * (ya + yb) + ya * cb)) / 4;
        yy = 3 * (3 * ab * (3 * xb * xb - xa * xa - 2 * xa * xc) - xa * (3 * ac * (xa + xb) + xa * cb)) / 4;
        xy = xa * ya * (6 * ab + 6 * ac - 3 * bc + cb);
        ac = ya * ya;
        cb = xa * xa;
        xy = 3 * (xy + 9 * f * (cb * yb * yc - xb * xc * ac) - 18 * xb * yb * ab) / 8;

        if (ex < 0) /* negate values if inside self-intersection loop */
        {
            dx = -dx;
            dy = -dy;
            xx = -xx;
            yy = -yy;
            xy = -xy;
            ac = -ac;
            cb = -cb;
        } /* init differences of 3rd degree */
        ab = 6 * ya * ac;
        ac = -6 * xa * ac;
        bc = 6 * ya * cb;
        cb = -6 * xa * cb;
        dx += xy;
        ex = dx + dy;
        dy += xy; /* error of 1st step */

        for (pxy = &xy, fx = fy = f; x0 != x3 && y0 != y3;)
        {
            TURBO_SET_PIXEL_BOUNDS(xOrigin + x0 * xScale, yOrigin + y0 * yScale, col); /* draw curve */
            do                                                                         /* move sub-steps of one pixel */
            {
                if (dx > *pxy || dy < *pxy)
                {
                    goto exit;
                }
                /* confusing values */
                y1 = 2 * ex - dy; /* save value for test of y step */
                if (2 * ex >= dx) /* x sub-step */
                {
                    fx--;
                    ex += dx += xx;
                    dy += xy += ac;
                    yy += bc;
                    xx += ab;
                }
                if (y1 <= 0) /* y sub-step */
                {
                    fy--;
                    ex += dy += yy;
                    dx += xy += bc;
                    xx += ac;
                    yy += cb;
                }
            } while (fx > 0 && fy > 0); /* pixel complete? */
            if (2 * fx <= f)
            {
                x0 += sx;
                fx += f;
            } /* x step */
            if (2 * fy <= f)
            {
                y0 += sy;
                fy += f;
            } /* y step */
            if (pxy == &xy && dx < 0 && dy > 0)
            {
                pxy = &EP; /* pixel ahead valid */
            }
        }
    exit:
        xx = x0;
        x0 = x3;
        x3 = xx;
        sx = -sx;
        xb = -xb; /* swap legs */
        yy = y0;
        y0 = y3;
        y3 = yy;
        sy = -sy;
        yb = -yb;
        x1 = x2;
    } while (leg--); /* try other end */
    drawLineInner(x0, y0, x3, y3, col, 0, xOrigin, yOrigin, xScale,
                  yScale); /* remaining part in case of cusp or crunode */
}

/**
 * @brief Draw limited cubic Bezier segment
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param x3 The X coordinate of the fourth point
 * @param y3 The Y coordinate of the fourth point
 * @param col The color to draw
 */
void drawCubicBezierSeg(int x0, int y0, float x1, float y1, float x2, float y2, int x3, int y3, paletteColor_t col)
{
    drawCubicBezierSegInner(x0, y0, x1, y1, x2, y2, x3, y3, col, 0, 0, 1, 1);
}

/**
 * @brief Helper function to draw any cubic Bezier curve
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param x3 The X coordinate of the fourth point
 * @param y3 The Y coordinate of the fourth point
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
static void drawCubicBezierInner(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col,
                                 int xOrigin, int yOrigin, int xScale, int yScale)
{
    int n = 0, i = 0;
    long xc = x0 + x1 - x2 - x3, xa = xc - 4 * (x1 - x2);
    long xb = x0 - x1 - x2 + x3, xd = xb + 4 * (x1 + x2);
    long yc = y0 + y1 - y2 - y3, ya = yc - 4 * (y1 - y2);
    long yb = y0 - y1 - y2 + y3, yd = yb + 4 * (y1 + y2);
    float fx0 = x0, fy0 = y0;
    double t1 = xb * xb - xa * xc, t2, t[5];
    /* sub-divide curve at gradient sign changes */
    if (xa == 0) /* horizontal */
    {
        if (labs(xc) < 2 * labs(xb))
        {
            t[n++] = xc / (2.0 * xb); /* one change */
        }
    }
    else if (t1 > 0.0) /* two changes */
    {
        t2 = sqrt(t1);
        t1 = (xb - t2) / xa;
        if (fabs(t1) < 1.0)
        {
            t[n++] = t1;
        }
        t1 = (xb + t2) / xa;
        if (fabs(t1) < 1.0)
        {
            t[n++] = t1;
        }
    }
    t1 = yb * yb - ya * yc;
    if (ya == 0) /* vertical */
    {
        if (labs(yc) < 2 * labs(yb))
        {
            t[n++] = yc / (2.0 * yb); /* one change */
        }
    }
    else if (t1 > 0.0) /* two changes */
    {
        t2 = sqrt(t1);
        t1 = (yb - t2) / ya;
        if (fabs(t1) < 1.0)
        {
            t[n++] = t1;
        }
        t1 = (yb + t2) / ya;
        if (fabs(t1) < 1.0)
        {
            t[n++] = t1;
        }
    }
    for (i = 1; i < n; i++) /* bubble sort of 4 points */
        if ((t1 = t[i - 1]) > t[i])
        {
            t[i - 1] = t[i];
            t[i]     = t1;
            i        = 0;
        }

    t1   = -1.0;
    t[n] = 1.0;              /* begin / end point */
    for (i = 0; i <= n; i++) /* draw each segment separately */
    {
        t2        = t[i]; /* sub-divide at t[i-1], t[i] */
        float fx1 = (t1 * (t1 * xb - 2 * xc) - t2 * (t1 * (t1 * xa - 2 * xb) + xc) + xd) / 8 - fx0;
        float fy1 = (t1 * (t1 * yb - 2 * yc) - t2 * (t1 * (t1 * ya - 2 * yb) + yc) + yd) / 8 - fy0;
        float fx2 = (t2 * (t2 * xb - 2 * xc) - t1 * (t2 * (t2 * xa - 2 * xb) + xc) + xd) / 8 - fx0;
        float fy2 = (t2 * (t2 * yb - 2 * yc) - t1 * (t2 * (t2 * ya - 2 * yb) + yc) + yd) / 8 - fy0;
        float fx3, fy3;
        fx0 -= fx3 = (t2 * (t2 * (3 * xb - t2 * xa) - 3 * xc) + xd) / 8;
        fy0 -= fy3 = (t2 * (t2 * (3 * yb - t2 * ya) - 3 * yc) + yd) / 8;
        x3         = floor(fx3 + 0.5);
        y3         = floor(fy3 + 0.5); /* scale bounds to int */
        if (fx0 != 0.0)
        {
            fx1 *= fx0 = (x0 - x3) / fx0;
            fx2 *= fx0;
        }
        if (fy0 != 0.0)
        {
            fy1 *= fy0 = (y0 - y3) / fy0;
            fy2 *= fy0;
        }
        if (x0 != x3 || y0 != y3) /* segment t1 - t2 */
        {
            drawCubicBezierSegInner(x0, y0, x0 + fx1, y0 + fy1, x0 + fx2, y0 + fy2, x3, y3, col, xOrigin, yOrigin,
                                    xScale, yScale);
        }
        x0  = x3;
        y0  = y3;
        fx0 = fx3;
        fy0 = fy3;
        t1  = t2;
    }
}

/**
 * @brief Draw any cubic Bezier curve
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param x3 The X coordinate of the fourth point
 * @param y3 The Y coordinate of the fourth point
 * @param col The color to draw
 */
void drawCubicBezier(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col)
{
    drawCubicBezierInner(x0, y0, x1, y1, x2, y2, x3, y3, col, 0, 0, 1, 1);
}

/**
 * @brief Draw any cubic Bezier curve with translation and scaling
 *
 * @param x0 The X coordinate of the first point
 * @param y0 The Y coordinate of the first point
 * @param x1 The X coordinate of the second point
 * @param y1 The Y coordinate of the second point
 * @param x2 The X coordinate of the third point
 * @param y2 The Y coordinate of the third point
 * @param x3 The X coordinate of the fourth point
 * @param y3 The Y coordinate of the fourth point
 * @param col The color to draw
 * @param xOrigin The X-origin, in display pixels, of the scaled pixel area
 * @param yOrigin The Y-origin, in display pixels, of the scaled pixel area
 * @param xScale The width of each scaled pixel
 * @param yScale The height of each scaled pixel
 */
void drawCubicBezierScaled(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, paletteColor_t col,
                           int xOrigin, int yOrigin, int xScale, int yScale)
{
    for (uint8_t i = 0; i < xScale * yScale; i++)
    {
        drawCubicBezierInner(x0, y0, x1, y1, x2, y2, x3, y3, col, xOrigin + i % yScale, yOrigin + i / xScale, xScale,
                             yScale);
    }
}

/**
 * @brief Draw a quadratic spline. This overwrites the input arrays x and y.
 *
 * @param n The number of points in the spline
 * @param x The X coordinates of the points in the spline
 * @param y The Y coordinates of the points in the spline
 * @param col The color to draw
 */
void drawQuadSpline(int n, int x[], int y[], paletteColor_t col)
{
#define M_MAX 6
    float mi = 1, m[M_MAX]; /* diagonal constants of matrix */
    int i, x0, y0, x1, y1, x2 = x[n], y2 = y[n];

    assert(n > 1); /* need at least 3 points P[0]..P[n] */

    x[1] = x0 = 8 * x[1] - 2 * x[0]; /* first row of matrix */
    y[1] = y0 = 8 * y[1] - 2 * y[0];

    for (i = 2; i < n; i++) /* forward sweep */
    {
        if (i - 2 < M_MAX)
        {
            m[i - 2] = mi = 1.0 / (6.0 - mi);
        }
        x[i] = x0 = floor(8 * x[i] - x0 * mi + 0.5); /* store yi */
        y[i] = y0 = floor(8 * y[i] - y0 * mi + 0.5);
    }
    x1 = floor((x0 - 2 * x2) / (5.0 - mi) + 0.5); /* correction last row */
    y1 = floor((y0 - 2 * y2) / (5.0 - mi) + 0.5);

    for (i = n - 2; i > 0; i--) /* back substitution */
    {
        if (i <= M_MAX)
        {
            mi = m[i - 1];
        }
        x0 = floor((x[i] - x1) * mi + 0.5); /* next corner */
        y0 = floor((y[i] - y1) * mi + 0.5);
        drawQuadBezier((x0 + x1) / 2, (y0 + y1) / 2, x1, y1, x2, y2, col);
        x2 = (x0 + x1) / 2;
        x1 = x0;
        y2 = (y0 + y1) / 2;
        y1 = y0;
    }
    drawQuadBezier(x[0], y[0], x1, y1, x2, y2, col);
}

/**
 * @brief Draw a cubic spline. This overwrites the input arrays x and y
 *
 * @param n The number of points in the spline
 * @param x The X coordinates of the points in the spline
 * @param y The Y coordinates of the points in the spline
 * @param col The color to draw
 */
void drawCubicSpline(int n, int x[], int y[], paletteColor_t col)
{
#define M_MAX 6
    float mi = 0.25, m[M_MAX] = {0}; /* diagonal constants of matrix */
    int x3 = x[n - 1], y3 = y[n - 1], x4 = x[n], y4 = y[n];
    int i, x0, y0, x1, y1, x2, y2;

    assert(n > 2); /* need at least 4 points P[0]..P[n] */

    x[1] = x0 = 12 * x[1] - 3 * x[0]; /* first row of matrix */
    y[1] = y0 = 12 * y[1] - 3 * y[0];

    for (i = 2; i < n; i++) /* foreward sweep */
    {
        if (i - 2 < M_MAX)
        {
            m[i - 2] = mi = 0.25 / (2.0 - mi);
        }
        x[i] = x0 = floor(12 * x[i] - 2 * x0 * mi + 0.5);
        y[i] = y0 = floor(12 * y[i] - 2 * y0 * mi + 0.5);
    }
    x2 = floor((x0 - 3 * x4) / (7 - 4 * mi) + 0.5); /* correct last row */
    y2 = floor((y0 - 3 * y4) / (7 - 4 * mi) + 0.5);
    drawCubicBezier(x3, y3, (x2 + x4) / 2, (y2 + y4) / 2, x4, y4, x4, y4, col);

    if (n - 3 < M_MAX)
    {
        mi = m[n - 3];
    }
    x1 = floor((x[n - 2] - 2 * x2) * mi + 0.5);
    y1 = floor((y[n - 2] - 2 * y2) * mi + 0.5);
    for (i = n - 3; i > 0; i--) /* back substitution */
    {
        if (i <= M_MAX)
        {
            mi = m[i - 1];
        }
        x0 = floor((x[i] - 2 * x1) * mi + 0.5);
        y0 = floor((y[i] - 2 * y1) * mi + 0.5);
        x4 = floor((x0 + 4 * x1 + x2 + 3) / 6.0); /* reconstruct P[i] */
        y4 = floor((y0 + 4 * y1 + y2 + 3) / 6.0);
        drawCubicBezier(x4, y4, floor((2 * x1 + x2) / 3 + 0.5), floor((2 * y1 + y2) / 3 + 0.5),
                        floor((x1 + 2 * x2) / 3 + 0.5), floor((y1 + 2 * y2) / 3 + 0.5), x3, y3, col);
        x3 = x4;
        y3 = y4;
        x2 = x1;
        y2 = y1;
        x1 = x0;
        y1 = y0;
    }
    x0 = x[0];
    x4 = floor((3 * x0 + 7 * x1 + 2 * x2 + 6) / 12.0); /* reconstruct P[1] */
    y0 = y[0];
    y4 = floor((3 * y0 + 7 * y1 + 2 * y2 + 6) / 12.0);
    drawCubicBezier(x4, y4, floor((2 * x1 + x2) / 3 + 0.5), floor((2 * y1 + y2) / 3 + 0.5),
                    floor((x1 + 2 * x2) / 3 + 0.5), floor((y1 + 2 * y2) / 3 + 0.5), x3, y3, col);
    drawCubicBezier(x0, y0, x0, y0, (x0 + x1) / 2, (y0 + y1) / 2, x4, y4, col);
}
