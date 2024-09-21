#include "wsgPalette.h"
#include "hdw-tft.h"
#include "trigonometry.h"

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

void drawWsgPalette(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette, bool flipLR, bool flipUD, int16_t rotateDeg)
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
                // Get colors from remap
                uint8_t color = palette->replacedColors[linein[srcX]];

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

void wsgPaletteReset(wsgPalette_t* palette)
{
    // Reset the palette
    for (int32_t i = 0; i < 217; i++)
    {
        palette->newColors[i] = i;
        palette->replacedColors[i] = i;
    }
}

void wsgPaletteGenerate(wsgPalette_t* palette)
{
    // Set new colors
    for (int32_t i = 0; i < 217; i++)
    {
        palette->replacedColors[i] = palette->newColors[i];
    }
}

void wsgPaletteSet(wsgPalette_t* palette, paletteColor_t replacedColor, paletteColor_t newColor)
{
    palette->newColors[replacedColor] = newColor;
}
