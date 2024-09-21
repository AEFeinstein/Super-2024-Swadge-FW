//==============================================================================
// Includes
//==============================================================================

#include <string.h>

#include "hdw-tft.h"
#include "macros.h"
#include "trigonometry.h"
#include "fill.h"
#include "wsg.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void rotatePixel(int16_t* x, int16_t* y, int16_t rotateDeg, int16_t width, int16_t height);

//==============================================================================
// Functions
//==============================================================================

/**
 * Transform a pixel's coordinates by rotation around the sprite's center point,
 * then reflection over Y axis, then reflection over X axis, then translation
 *
 * @param x The x coordinate of the pixel location to transform
 * @param y The y coordinate of the pixel location to transform
 * @param rotateDeg The number of degrees to rotate clockwise, must be 0-359
 * @param width  The width of the image
 * @param height The height of the image
 */
static void rotatePixel(int16_t* x, int16_t* y, int16_t rotateDeg, int16_t width, int16_t height)
{
    //  This function has been micro optimized by cnlohr on 2022-09-07, using gcc version 8.4.0 (crosstool-NG
    //  esp-2021r2-patch3)

    int32_t wx = *x;
    int32_t wy = *y;
    // First rotate the sprite around the sprite's center point

    // This solves the aliasing problem, but because of tan() it's only safe
    // to rotate by 0 to 90 degrees. So rotate by a multiple of 90 degrees
    // first, which doesn't need trig, then rotate the rest with shears
    // See http://datagenetics.com/blog/august32013/index.html
    // See https://graphicsinterface.org/wp-content/uploads/gi1986-15.pdf

    int32_t tx = -(width / 2);
    int32_t ty = -(height / 2);

    // Center around (0, 0)
    wx += tx;
    wy += ty;

    // First rotate to the nearest 90 degree boundary, which is trivial
    if (rotateDeg >= 270)
    {
        // (x, y) -> (y, -x)
        int16_t tmp = wx;
        wx          = wy;
        wy          = 2 * tx - tmp + width - 1;
        rotateDeg -= 270;
    }
    else if (rotateDeg >= 180)
    {
        // (x, y) -> (-x, -y)
        wx = 2 * tx - wx + width - 1;
        wy = 2 * ty - wy + height - 1;
        rotateDeg -= 180;
    }
    else if (rotateDeg >= 90)
    {
        // (x, y) -> (-y, x)
        int16_t tmp = wx;
        wx          = 2 * ty - wy + height - 1;
        wy          = tmp;
        rotateDeg -= 90;
    }
    // Now that it's rotated to a 90 degree boundary, find out how much more
    // there is to rotate by shearing

    // If there's any more to rotate, apply three shear matrices in order
    if (0 < rotateDeg && rotateDeg < 90)
    {
        // 1st shear
        wx = wx - ((wy * tan1024[rotateDeg / 2]) + 512) / 1024;
        // 2nd shear
        wy = ((wx * sin1024[rotateDeg]) + 512) / 1024 + wy;
        // 3rd shear
        wx = wx - ((wy * tan1024[rotateDeg / 2]) + 512) / 1024;
    }

    // Return pixel to original position
    wx -= tx;
    wy -= ty;

    *x = wx;
    *y = wy;
}

/**
 * @brief Draw a WSG to the display
 *
 * @param wsg  The WSG to draw to the display
 * @param xOff The x offset to draw the WSG at
 * @param yOff The y offset to draw the WSG at
 * @param flipLR true to flip the image across the Y axis
 * @param flipUD true to flip the image across the X axis
 * @param rotateDeg The number of degrees to rotate clockwise, must be 0-359
 */
void drawWsg(const wsg_t* wsg, int16_t xOff, int16_t yOff, bool flipLR, bool flipUD, int16_t rotateDeg)
{
    //  This function has been micro optimized by cnlohr on 2022-09-08, using gcc version 8.4.0 (crosstool-NG
    //  esp-2021r2-patch3)

    /* Btw quick test code for second case is:
    for( int mode = 0; mode < 8; mode++ )
    {
        drawWsgLocal( &example_sprite, 50+mode*20, (global_i%20)-10, !!(mode&1), !!(mode & 2), (mode & 4)*10);
        drawWsgLocal( &example_sprite, 50+mode*20, (global_i%20)+230, !!(mode&1), !!(mode & 2), (mode & 4)*10);
        drawWsgLocal( &example_sprite, (global_i%20)-10, 50+mode*20, !!(mode&1), !!(mode & 2), (mode & 4)*10);
        drawWsgLocal( &example_sprite, (global_i%20)+270, 50+mode*20, !!(mode&1), !!(mode & 2), (mode & 4)*10);
    }
    */

    if (NULL == wsg->px)
    {
        return;
    }

    if (rotateDeg)
    {
        SETUP_FOR_TURBO();
        uint32_t wsgw = wsg->w;
        uint32_t wsgh = wsg->h;
        for (int32_t srcY = 0; srcY < wsgh; srcY++)
        {
            int32_t usey = srcY;

            // Reflect over X axis?
            if (flipUD)
            {
                usey = wsg->h - 1 - usey;
            }

            const paletteColor_t* linein = &wsg->px[usey * wsgw];

            // Reflect over Y axis?
            uint32_t readX    = 0;
            uint32_t advanceX = 1;
            if (flipLR)
            {
                readX    = wsgw - 1;
                advanceX = -1;
            }

            int32_t localX = 0;
            for (int32_t srcX = 0; srcX != wsgw; srcX++)
            {
                // Draw if not transparent
                uint8_t color = linein[readX];
                if (cTransparent != color)
                {
                    uint16_t tx = localX;
                    uint16_t ty = srcY;

                    rotatePixel((int16_t*)&tx, (int16_t*)&ty, rotateDeg, wsgw, wsgh);
                    tx += xOff;
                    ty += yOff;
                    TURBO_SET_PIXEL_BOUNDS(tx, ty, color);
                }
                localX++;
                readX += advanceX;
            }
        }
    }
    else
    {
        // Draw the image's pixels (no rotation or transformation)
        uint32_t w         = TFT_WIDTH;
        paletteColor_t* px = getPxTftFramebuffer();

        uint16_t wsgw = wsg->w;
        uint16_t wsgh = wsg->h;

        int32_t xstart = 0;
        int16_t xend   = wsgw;
        int32_t xinc   = 1;

        // Reflect over Y axis?
        if (flipLR)
        {
            xstart = wsgw - 1;
            xend   = -1;
            xinc   = -1;
        }

        if (xOff < 0)
        {
            if (xinc > 0)
            {
                xstart -= xOff;
                if (xstart >= xend)
                {
                    return;
                }
            }
            else
            {
                xstart += xOff;
                if (xend >= xstart)
                {
                    return;
                }
            }
            xOff = 0;
        }

        if (xOff + wsgw > w)
        {
            int32_t peelBack = (xOff + wsgw) - w;
            if (xinc > 0)
            {
                xend -= peelBack;
                if (xstart >= xend)
                {
                    return;
                }
            }
            else
            {
                xend += peelBack;
                if (xend >= xstart)
                {
                    return;
                }
            }
        }

        for (int16_t srcY = 0; srcY < wsgh; srcY++)
        {
            int32_t usey = srcY;

            // Reflect over X axis?
            if (flipUD)
            {
                usey = wsgh - 1 - usey;
            }

            const paletteColor_t* linein = &wsg->px[usey * wsgw];

            // Transform this pixel's draw location as necessary
            uint32_t dstY = srcY + yOff;

            // It is too complicated to detect both directions and backoff correctly, so we just do this here.
            // It does slow things down a "tiny" bit.  People in the future could optimize out this check.
            if (dstY >= TFT_HEIGHT)
            {
                continue;
            }

            int32_t lineOffset = dstY * w;
            int32_t dstx       = xOff + lineOffset;

            for (int32_t srcX = xstart; srcX != xend; srcX += xinc)
            {
                // Draw if not transparent
                uint8_t color = linein[srcX];
                if (cTransparent != color)
                {
                    px[dstx] = color;
                }
                dstx++;
            }
        }
    }
}

/**
 * @brief Draw a WSG to the display without flipping or rotation
 *
 * @param wsg  The WSG to draw to the display
 * @param xOff The x offset to draw the WSG at
 * @param yOff The y offset to draw the WSG at
 */
void drawWsgSimple(const wsg_t* wsg, int16_t xOff, int16_t yOff)
{
    //  This function has been micro optimized by cnlohr on 2022-09-07, using gcc version 8.4.0 (crosstool-NG
    //  esp-2021r2-patch3)

    if (NULL == wsg->px)
    {
        return;
    }

    // Only draw in bounds
    int dWidth                   = TFT_WIDTH;
    int wWidth                   = wsg->w;
    int xMin                     = CLAMP(xOff, 0, dWidth);
    int xMax                     = CLAMP(xOff + wWidth, 0, dWidth);
    int yMin                     = CLAMP(yOff, 0, TFT_HEIGHT);
    int yMax                     = CLAMP(yOff + wsg->h, 0, TFT_HEIGHT);
    paletteColor_t* px           = getPxTftFramebuffer();
    int numX                     = xMax - xMin;
    int wsgY                     = (yMin - yOff);
    int wsgX                     = (xMin - xOff);
    paletteColor_t* lineout      = &px[(yMin * dWidth) + xMin];
    const paletteColor_t* linein = &wsg->px[wsgY * wWidth + wsgX];

    // Draw each pixel
    for (int y = yMin; y < yMax; y++)
    {
        for (int x = 0; x < numX; x++)
        {
            int color = linein[x];
            if (color != cTransparent)
            {
                lineout[x] = color;
            }
        }
        lineout += dWidth;
        linein += wWidth;
        wsgY++;
    }
}

/**
 * @brief Draw a WSG to the display without flipping or rotation
 *
 * @param wsg  The WSG to draw to the display
 * @param xOff The x offset to draw the WSG at
 * @param yOff The y offset to draw the WSG at
 * @param xScale The amount to scale the image horizontally
 * @param yScale The amount to scale the image vertically
 */
void drawWsgSimpleScaled(const wsg_t* wsg, int16_t xOff, int16_t yOff, int16_t xScale, int16_t yScale)
{
    //  This function has been micro optimized by cnlohr on 2022-09-07, using gcc version 8.4.0 (crosstool-NG
    //  esp-2021r2-patch3)

    if (NULL == wsg->px)
    {
        return;
    }

    // Only draw in bounds
    int dWidth                   = TFT_WIDTH;
    int dHeight                  = TFT_HEIGHT;
    int wWidth                   = wsg->w;
    int xMax                     = CLAMP(xOff + wWidth * xScale, 0, dWidth);
    int yMax                     = CLAMP(yOff + wsg->h * yScale, 0, dHeight);
    const paletteColor_t* linein = wsg->px;

    int x1;
    int y1;

    // Draw each pixel, scaled
    for (int y = yOff, iy = 0; y < yMax && iy < wsg->h; y += yScale, iy++)
    {
        if (y >= TFT_HEIGHT)
        {
            return;
        }

        y1 = y + yScale;

        // Entire "pixel" is off-screen
        if (y1 <= 0)
        {
            linein += wWidth;
            continue;
        }

        for (int x = xOff, ix = 0; x < xMax && ix < wsg->w; x += xScale, ix++)
        {
            if (x >= TFT_WIDTH)
            {
                // next line
                break;
            }

            x1 = x + xScale;

            if (x1 <= 0)
            {
                // next pixel
                continue;
            }

            int color = linein[ix];
            if (color != cTransparent)
            {
                fillDisplayArea(MAX(x, 0), MAX(y, 0), MIN(x1, dWidth), MIN(y1, dHeight), color);
            }
        }
        linein += wWidth;
    }
}

/**
 * @brief Draw a WSG to the display without flipping or rotation at half size
 *
 * @param wsg  The WSG to draw to the display
 * @param xOff The x offset to draw the WSG at
 * @param yOff The y offset to draw the WSG at
 */
void drawWsgSimpleHalf(const wsg_t* wsg, int16_t xOff, int16_t yOff)
{
    //  This function has been micro optimized by cnlohr on 2022-09-07, using gcc version 8.4.0 (crosstool-NG
    //  esp-2021r2-patch3)

    if (NULL == wsg->px)
    {
        return;
    }

    // Only draw in bounds
    int dWidth                   = TFT_WIDTH;
    int wWidth                   = wsg->w;
    int xMin                     = CLAMP(xOff, 0, dWidth);
    int xMax                     = CLAMP(xOff + (wWidth / 2), 0, dWidth);
    int yMin                     = CLAMP(yOff, 0, TFT_HEIGHT);
    int yMax                     = CLAMP(yOff + (wsg->h / 2), 0, TFT_HEIGHT);
    paletteColor_t* px           = getPxTftFramebuffer();
    int numX                     = xMax - xMin;
    int wsgY                     = (yMin - yOff);
    int wsgX                     = (xMin - xOff);
    paletteColor_t* lineout      = &px[(yMin * dWidth) + xMin];
    const paletteColor_t* linein = &wsg->px[wsgY * wWidth + wsgX];

    // Draw each pixel
    for (int y = yMin; y < yMax; y++)
    {
        for (int x = 0; x < numX; x++)
        {
            int color = linein[x * 2];
            if (color != cTransparent)
            {
                lineout[x] = color;
            }
        }
        lineout += dWidth;
        linein += (2 * wWidth);
        wsgY++;
    }
}

/**
 * @brief Quickly copy a WSG to the display without flipping or rotation or transparency
 *
 * If the source WSG does have transparency, an indeterminate color will be drawn to the TFT
 *
 * @param wsg  The WSG to draw to the display
 * @param xOff The x offset to draw the WSG at
 * @param yOff The y offset to draw the WSG at
 */
void drawWsgTile(const wsg_t* wsg, int32_t xOff, int32_t yOff)
{
    if (xOff > TFT_WIDTH)
    {
        return;
    }

    // Bound in the Y direction
    int32_t yStart = (yOff < 0) ? 0 : yOff;
    int32_t yEnd   = ((yOff + wsg->h) > TFT_HEIGHT) ? TFT_HEIGHT : (yOff + wsg->h);

    int wWidth                  = wsg->w;
    int dWidth                  = TFT_WIDTH;
    const paletteColor_t* pxWsg = &wsg->px[(yOff < 0) ? (wsg->h - (yEnd - yStart)) * wWidth : 0];
    paletteColor_t* pxDisp      = &(getPxTftFramebuffer()[yStart * dWidth + xOff]);

    // Bound in the X direction
    int32_t copyLen = wsg->w;
    if (xOff < 0)
    {
        copyLen += xOff;
        pxDisp -= xOff;
        pxWsg -= xOff;
        xOff = 0;
    }

    if (xOff + copyLen > TFT_WIDTH)
    {
        copyLen = TFT_WIDTH - xOff;
    }

    // copy each row
    for (int32_t y = yStart; y < yEnd; y++)
    {
        // Copy the row
        // probably faster if we can guarantee copyLen is a multiple of 4
        memcpy(pxDisp, pxWsg, copyLen);
        pxDisp += dWidth;
        pxWsg += wWidth;
    }
}