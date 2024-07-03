//==============================================================================
// Includes
//==============================================================================

#include <string.h>

#include "hdw-tft.h"
#include "macros.h"
#include "fill.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void _floodFill(uint16_t x, uint16_t y, paletteColor_t search, paletteColor_t fill, uint16_t xMin, uint16_t yMin,
                       uint16_t xMax, uint16_t yMax);
static void _floodFillInner(uint16_t x, uint16_t y, paletteColor_t search, paletteColor_t fill, uint16_t xMin,
                            uint16_t yMin, uint16_t xMax, uint16_t yMax);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Fill a rectangular area on a display with a single color
 *
 * @param x1 The x coordinate to start the fill (top left)
 * @param y1 The y coordinate to start the fill (top left)
 * @param x2 The x coordinate to stop the fill (bottom right)
 * @param y2 The y coordinate to stop the fill (bottom right)
 * @param c  The color to fill
 */
void fillDisplayArea(int16_t x1, int16_t y1, int16_t x2, int16_t y2, paletteColor_t c)
{
    // Note: int16_t vs int data types tested for speed.
    // This function has been micro optimized by cnlohr on 2022-09-07,
    // using gcc version 8.4.0 (crosstool-NG esp-2021r2-patch3)

    // Only draw on the display
    int xMin = CLAMP(x1, 0, TFT_WIDTH);
    int xMax = CLAMP(x2, 0, TFT_WIDTH);
    int yMin = CLAMP(y1, 0, TFT_HEIGHT);
    int yMax = CLAMP(y2, 0, TFT_HEIGHT);

    uint32_t dw         = TFT_WIDTH;
    paletteColor_t* pxs = getPxTftFramebuffer() + yMin * dw + xMin;
    int copyLen         = xMax - xMin;

    // Set each pixel
    for (int y = yMin; y < yMax; y++)
    {
        memset(pxs, c, copyLen);
        pxs += dw;
    }
}

/**
 * 'Shade' an area by drawing pixels over it in a ordered-dithering way
 *
 * @param x1 The X pixel to start at
 * @param y1 The Y pixel to start at
 * @param x2 The X pixel to end at
 * @param y2 The Y pixel to end at
 * @param shadeLevel The level of shading, Higher means more shaded. Must be 0 to 4
 * @param color the color to draw with
 */
void shadeDisplayArea(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t shadeLevel, paletteColor_t color)
{
    SETUP_FOR_TURBO();
    int16_t xMin, yMin, xMax, yMax;
    if (x1 < x2)
    {
        xMin = x1;
        xMax = x2;
    }
    else
    {
        xMin = x2;
        xMax = x1;
    }
    if (y1 < y2)
    {
        yMin = y1;
        yMax = y2;
    }
    else
    {
        yMin = y2;
        yMax = y1;
    }

    if (xMin < 0)
    {
        xMin = 0;
    }
    if (xMax >= (int16_t)TFT_WIDTH)
    {
        xMax = TFT_WIDTH - 1;
    }
    if (xMin >= (int16_t)TFT_WIDTH)
    {
        return;
    }
    if (xMax < 0)
    {
        return;
    }
    if (yMin < 0)
    {
        yMin = 0;
    }
    if (yMax >= (int16_t)TFT_HEIGHT)
    {
        yMax = TFT_HEIGHT - 1;
    }
    if (yMin >= (int16_t)TFT_HEIGHT)
    {
        return;
    }
    if (yMax < 0)
    {
        return;
    }

    for (int16_t dy = yMin; dy <= yMax; dy++)
    {
        for (int16_t dx = xMin; dx < xMax; dx++)
        {
            switch (shadeLevel)
            {
                case 0:
                {
                    // 25% faded
                    if (dy % 2 == 0 && dx % 2 == 0)
                    {
                        TURBO_SET_PIXEL_BOUNDS(dx, dy, color);
                    }
                    break;
                }
                case 1:
                {
                    // 37.5% faded
                    if (dy % 2 == 0 && dx % 2 == 0)
                    {
                        TURBO_SET_PIXEL_BOUNDS(dx, dy, color);
                    }
                    else if (dx % 4 == 0)
                    {
                        TURBO_SET_PIXEL_BOUNDS(dx, dy, color);
                    }
                    break;
                }
                case 2:
                {
                    // 50% faded
                    if ((dy % 2) == (dx % 2))
                    {
                        TURBO_SET_PIXEL_BOUNDS(dx, dy, color);
                    }
                    break;
                }
                case 3:
                {
                    // 62.5% faded
                    if (dy % 2 == 0 && dx % 2 == 0)
                    {
                        TURBO_SET_PIXEL_BOUNDS(dx, dy, color);
                    }
                    else if (dx % 4 < 3)
                    {
                        TURBO_SET_PIXEL_BOUNDS(dx, dy, color);
                    }
                    break;
                }
                case 4:
                {
                    // 75% faded
                    if (dy % 2 == 0 || dx % 2 == 0)
                    {
                        TURBO_SET_PIXEL_BOUNDS(dx, dy, color);
                    }
                    break;
                }
                default:
                {
                    return;
                }
            }
        }
    }
}

/**
 * @brief Attempt to fill a convex shape bounded by a border of a given color using
 * the even-odd rule
 *
 * https://en.wikipedia.org/wiki/Even%E2%80%93odd_rule
 *
 * WARNING!!! This is very finicky and is not guaranteed to work in all cases.
 *
 * This iterates over each row in the bounding box, top to bottom, left to right
 *
 * This assumes that each row starts outside or on the boundary of the shape to
 * be filled.
 *
 * Each time a pixel of the 'boundary color' is iterated over, the in/out
 * boolean will flip. Thick boundaries will have hysteresis, but shapes with
 * concave features will have undrawn rows because the function checks for
 * an even number of transitions within a row before drawing.
 *
 * @param x0 The left index of the bounding box
 * @param y0 The top index of the bounding box
 * @param x1 The right index of the bounding box
 * @param y1 The bottom index of the bounding box
 * @param boundaryColor The color of the boundary to fill in
 * @param fillColor The color to fill
 */
void oddEvenFill(int x0, int y0, int x1, int y1, paletteColor_t boundaryColor, paletteColor_t fillColor)
{
    SETUP_FOR_TURBO();

    // Adjust the bounding box if it's out of bounds
    if (x0 < 0)
    {
        x0 = 0;
    }
    if (x1 > TFT_WIDTH)
    {
        x1 = TFT_WIDTH;
    }
    if (y0 < 0)
    {
        y0 = 0;
    }
    if (y1 > TFT_HEIGHT)
    {
        y1 = TFT_HEIGHT;
    }
    for (int y = y0; y < y1; y++)
    {
        // Assume starting outside the shape or on border for each row
        // Count rising edges of row
        bool isInside            = false;
        bool insideHysteresis    = false;
        uint16_t transitionCount = 0;
        // Pre-scan the row for even number of transitions. Algo only works for even number of transitions.
        for (int x = x0; x < x1; x++)
        {
            if (boundaryColor == getPxTft(x, y))
            {
                // Flip this boolean, don't color the boundary
                if (!insideHysteresis)
                {
                    isInside         = !isInside;
                    insideHysteresis = true;
                    transitionCount++;
                }
            }
            else
            {
                insideHysteresis = false; // If not on a boundary color, reset hysteresis
            }
        }
        if (!(transitionCount % 2)) // Check for even number of transitions to prevent coloring to edge of bounding box.
        {
            isInside         = false;
            insideHysteresis = false;
            for (int x = x0; x < x1; x++)
            {
                // If a boundary is hit
                if (boundaryColor == getPxTft(x, y))
                {
                    // Flip this boolean, don't color the boundary
                    if (!insideHysteresis)
                    {
                        isInside         = !isInside;
                        insideHysteresis = true;
                    }
                }
                else if (isInside)
                {
                    // If we're in-bounds, color the pixel
                    TURBO_SET_PIXEL_BOUNDS(x, y, fillColor);
                    insideHysteresis = false;
                }
                else
                {
                    insideHysteresis = false;
                }
            }
        }
    }
}

/**
 * This is a recursive flood fill algorithm. It starts at the given coordinate, and will replace the color at
 * that coordinate, and all adjacent pixels with the same color, with the fill color.
 *
 * The flood is also bounded wthin the given rectangle.
 *
 * Note that the recursive algorithm is relatively slow and uses a lot of stack memory, so try not to use it when
 * possible.
 *
 * This is adapted from http://www.adammil.net/blog/v126_A_More_Efficient_Flood_Fill.html
 *
 * @param x The X coordinate to start the fill at
 * @param y The Y coordinate to start the fill at
 * @param col The color to fill in
 * @param xMin The minimum X coordinate to bound the fill
 * @param yMin The minimum Y coordinate to bound the fill
 * @param xMax The maximum X coordinate to bound the fill
 * @param yMax The maximum Y coordinate to bound the fill
 */
void floodFill(uint16_t x, uint16_t y, paletteColor_t col, uint16_t xMin, uint16_t yMin, uint16_t xMax, uint16_t yMax)
{
    if (getPxTft(x, y) == col)
    {
        // makes no sense to fill with the same color, so just don't
        return;
    }

    _floodFill(x, y, getPxTft(x, y), col, xMin, yMin, xMax, yMax);
}

/**
 * A helper function for floodFill(), a recursive flood fill algorithm. These two functions do basically the same thing
 * except this one accepts two colors, the color to replace and the color to replace with.
 *
 * @param x The X coordinate to start the fill at
 * @param y The Y coordinate to start the fill at
 * @param search The color to replace
 * @param fill The color to draw
 * @param xMin The minimum X coordinate to bound the fill
 * @param yMin The minimum Y coordinate to bound the fill
 * @param xMax The maximum X coordinate to bound the fill
 * @param yMax The maximum Y coordinate to bound the fill
 */
static void _floodFill(uint16_t x, uint16_t y, paletteColor_t search, paletteColor_t fill, uint16_t xMin, uint16_t yMin,
                       uint16_t xMax, uint16_t yMax)
{
    // at this point, we know array[y,x] is clear, and we want to move as far as possible to the upper-left. moving
    // up is much more important than moving left, so we could try to make this smarter by sometimes moving to
    // the right if doing so would allow us to move further up, but it doesn't seem worth the complexity
    while (true)
    {
        uint16_t ox = x, oy = y;
        while (y != yMin && getPxTft(x, y - 1) == search)
        {
            y--;
        }
        while (x != xMin && getPxTft(x - 1, y) == search)
        {
            x--;
        }
        if (x == ox && y == oy)
        {
            break;
        }
    }
    _floodFillInner(x, y, search, fill, xMin, yMin, xMax, yMax);
}

/**
 * A helper function for floodFill() and _floodFillInner(), a recursive flood fill algorithm. This does the heavy
 * lifting for the algorithm.
 *
 * @param x The X coordinate to start the fill at
 * @param y The Y coordinate to start the fill at
 * @param search The color to replace
 * @param fill The color to draw
 * @param xMin The minimum X coordinate to bound the fill
 * @param yMin The minimum Y coordinate to bound the fill
 * @param xMax The maximum X coordinate to bound the fill
 * @param yMax The maximum Y coordinate to bound the fill
 */
static void _floodFillInner(uint16_t x, uint16_t y, paletteColor_t search, paletteColor_t fill, uint16_t xMin,
                            uint16_t yMin, uint16_t xMax, uint16_t yMax)
{
    // at this point, we know that array[y,x] is clear, and array[y-1,x] and array[y,x-1] are set.
    // we'll begin scanning down and to the right, attempting to fill an entire rectangular block
    uint16_t lastRowLength = 0; // the number of cells that were clear in the last row we scanned
    do
    {
        uint16_t rowLength = 0,
                 sx        = x; // keep track of how long this row is. sx is the starting x for the main scan below
        // now we want to handle a case like |***|, where we fill 3 cells in the first row and then after we move to
        // the second row we find the first  | **| cell is filled, ending our rectangular scan. rather than handling
        // this via the recursion below, we'll increase the starting value of 'x' and reduce the last row length to
        // match. then we'll continue trying to set the narrower rectangular block
        if (lastRowLength != 0
            && getPxTft(x, y) != search) // if this is not the first row and the leftmost cell is filled...
        {
            do
            {
                if (--lastRowLength == 0)
                {
                    return; // shorten the row. if it's full, we're done
                }
            } while (getPxTft(++x, y) != search); // otherwise, update the starting point of the main scan to match
            sx = x;
        }
        // we also want to handle the opposite case, | **|, where we begin scanning a 2-wide rectangular block and
        // then find on the next row that it has     |***| gotten wider on the left. again, we could handle this
        // with recursion but we'd prefer to adjust x and lastRowLength instead
        else
        {
            for (; x != xMin && getPxTft(x - 1, y) == search; rowLength++, lastRowLength++)
            {
                setPxTft(--x, y, fill); // to avoid scanning the cells twice, we'll fill them and update rowLength here
                // if there's something above the new starting point, handle that recursively. this deals with cases
                // like |* **| when we begin filling from (2,0), move down to (2,1), and then move left to (0,1).
                // the  |****| main scan assumes the portion of the previous row from x to x+lastRowLength has already
                // been filled. adjusting x and lastRowLength breaks that assumption in this case, so we must fix it
                if (y != yMin && getPxTft(x, y - 1) == search)
                {
                    _floodFill(x, y - 1, search, fill, xMin, yMin, xMax,
                               yMax); // use _Fill since there may be more up and left
                }
            }
        }

        // now at this point we can begin to scan the current row in the rectangular block. the span of the previous
        // row from x (inclusive) to x+lastRowLength (exclusive) has already been filled, so we don't need to
        // check it. so scan across to the right in the current row
        for (; sx < xMax && getPxTft(sx, y) == search; rowLength++, sx++)
        {
            setPxTft(sx, y, fill);
        }
        // now we've scanned this row. if the block is rectangular, then the previous row has already been scanned,
        // so we don't need to look upwards and we're going to scan the next row in the next iteration so we don't
        // need to look downwards. however, if the block is not rectangular, we may need to look upwards or rightwards
        // for some portion of the row. if this row was shorter than the last row, we may need to look rightwards near
        // the end, as in the case of |*****|, where the first row is 5 cells long and the second row is 3 cells long.
        // we must look to the right  |*** *| of the single cell at the end of the second row, i.e. at (4,1)
        if (rowLength < lastRowLength)
        {
            for (int end = x + lastRowLength;
                 ++sx < end;) // 'end' is the end of the previous row, so scan the current row to
            {                 // there. any clear cells would have been connected to the previous
                if (getPxTft(sx, y) == search)
                {
                    _floodFillInner(sx, y, search, fill, xMin, yMin, xMax,
                                    yMax); // row. the cells up and left must be set so use FillCore
                }
            }
        }
        // alternately, if this row is longer than the previous row, as in the case |*** *| then we must look above
        // the end of the row, i.e at (4,0)                                         |*****|
        else if (rowLength > lastRowLength && y != yMin) // if this row is longer and we're not already at the top...
        {
            for (int ux = x + lastRowLength; ++ux < sx;) // sx is the end of the current row
            {
                if (getPxTft(ux, y - 1) == search)
                {
                    _floodFill(ux, y - 1, search, fill, xMin, yMin, xMax,
                               yMax); // since there may be clear cells up and left, use _Fill
                }
            }
        }
        lastRowLength = rowLength;              // record the new row length
    } while (lastRowLength != 0 && ++y < yMax); // if we get to a full row or to the bottom, we're done
}
