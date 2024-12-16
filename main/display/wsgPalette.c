/**
 * @file wsgPalette.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Provides palette swap functionality for Swadge
 * @version 1.0.0
 * @date 2024-09-20
 *
 * @copyright Copyright (c) 2024
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include "wsgPalette.h"
#include "hdw-tft.h"
#include "trigonometry.h"
#include "macros.h"
#include "fill.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draw a WSG to the display utilizing a palette
 *
 * @param wsg  The WSG to draw to the display
 * @param xOff The x offset to draw the WSG at
 * @param yOff The y offset to draw the WSG at
 * @param palette The new palette used to translate the colors
 * @param flipLR true to flip the image across the Y axis
 * @param flipUD true to flip the image across the X axis
 * @param rotateDeg The number of degrees to rotate clockwise, must be 0-359
 */
void drawWsgPalette(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette, bool flipLR, bool flipUD,
                    int16_t rotateDeg)
{
    //  This function has been micro optimized by cnlohr on 2022-09-08, using gcc version 8.4.0 (crosstool-NG
    //  esp-2021r2-patch3)

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
                uint8_t color = palette->newColors[linein[readX]];
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
                // Get colors from remap
                uint8_t color = palette->newColors[linein[srcX]];

                // Draw if not transparent
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
 * @param palette Color Map to use
 */
void drawWsgPaletteSimple(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette)
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
            uint8_t color = palette->newColors[linein[x]];
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
 * @param palette Color Map to use
 * @param xScale The amount to scale the image horizontally
 * @param yScale The amount to scale the image vertically
 */
void drawWsgPaletteSimpleScaled(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette, int16_t xScale,
                                int16_t yScale)
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

            uint8_t color = palette->newColors[linein[ix]];
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
 * @param palette Color Map to use
 */
void drawWsgPaletteSimpleHalf(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette)
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
            uint8_t color = palette->newColors[linein[x * 2]];
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
 * @brief Resets the palette to initial state
 *
 * @param palette Color map to modify
 */
void wsgPaletteReset(wsgPalette_t* palette)
{
    // Reset the palette
    for (int32_t i = 0; i < 217; i++)
    {
        palette->newColors[i] = i;
    }
}

/**
 * @brief Sets a single color to the provided palette
 *
 * @param palette Color map to modify
 * @param replacedColor Color to be replaced
 * @param newColor Color that will replace the previous
 */
void wsgPaletteSet(wsgPalette_t* palette, paletteColor_t replacedColor, paletteColor_t newColor)
{
    palette->newColors[replacedColor] = newColor;
}

void wsgPaletteSetGroup(wsgPalette_t* palette, paletteColor_t* replacedColors, paletteColor_t* newColors,
                        uint8_t arrSize)
{
    for (int32_t i = 0; i < arrSize; i++)
    {
        wsgPaletteSet(palette, replacedColors[i], newColors[i]);
    }
}