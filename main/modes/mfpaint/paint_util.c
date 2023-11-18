#include "paint_util.h"

#include "paint_common.h"
#include "paint_nvs.h"

#include "shapes.h"

paletteColor_t getContrastingColor(paletteColor_t col)
{
    uint32_t rgb = paletteToRGB(col);

    // TODO I guess this actually won't work at all on 50% gray, or that well on other grays
    uint8_t r            = 255 - (rgb & 0xFF);
    uint8_t g            = 255 - ((rgb >> 8) & 0xFF);
    uint8_t b            = 255 - ((rgb >> 16) & 0xFF);
    uint32_t contrastCol = (r << 16) | (g << 8) | (b);

    return RGBtoPalette(contrastCol);
}

paletteColor_t getContrastingColorBW(paletteColor_t col)
{
    uint32_t rgb = paletteToRGB(col);
    uint8_t r    = rgb & 0xFF;
    uint8_t g    = (rgb >> 8) & 0xFF;
    uint8_t b    = (rgb >> 16) & 0xFF;

    // TODO something with HSL but this pretty much works...
    return (r + g + b) / 3 > 76 ? c000 : c555;
}

void colorReplaceWsg(wsg_t* wsg, paletteColor_t find, paletteColor_t replace)
{
    for (uint16_t i = 0; i < wsg->h * wsg->w; i++)
    {
        if (wsg->px[i] == find)
        {
            wsg->px[i] = replace;
        }
    }
}

void paintPlotSquareWave(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t waveLength, paletteColor_t col,
                         int xTr, int yTr, int xScale, int yScale)
{
    uint16_t xDiff = (x0 < x1) ? x1 - x0 : x0 - x1;
    uint16_t yDiff = (y0 < y1) ? y1 - y0 : y0 - y1;

    // use the shortest axis as the wave size
    uint16_t waveHeight = (xDiff < yDiff) ? xDiff : yDiff;

    if (waveLength == 0)
    {
        waveLength = waveHeight;
    }

    uint16_t x = x0;
    uint16_t y = y0;
    uint16_t stop;

    int16_t xDir = (x0 < x1) ? 1 : -1;
    int16_t yDir = (y0 < y1) ? 1 : -1;

    if (waveHeight < 1 && waveLength < 1)
    {
        // (a 2xN square wave is just a 2-thick line)
        return;
    }

    if (xDiff > yDiff)
    {
        // Horizontal -- waveHeight is on Y axis
        stop  = x + waveLength * xDir;

        while (x != x1)
        {
            setPxScaled(x, y, col, xTr, yTr, xScale, yScale);

            if (x == stop)
            {
                drawLineScaled(x, y, x, y + yDir * waveHeight, col, 0, xTr, yTr, xScale, yScale);
                y += yDir * waveHeight;
                yDir = -yDir;
                stop = x + waveLength * xDir;
            }

            x += xDir;
        }
    }
    else
    {
        // Vertical -- waveHeight is on X axis
        stop  = y + waveLength * yDir;

        while (y != y1)
        {
            setPxScaled(x, y, col, xTr, yTr, xScale, yScale);

            if (y == stop)
            {
                drawLineScaled(x, y, x + xDir * waveHeight, y, col, 0, xTr, yTr, xScale, yScale);
                x += xDir * waveHeight;
                xDir = -xDir;
                stop = y + waveLength * yDir;
            }

            y += yDir;
        }
    }
}

void drawRectFilled(int x0, int y0, int x1, int y1, paletteColor_t col)
{
    if (x0 >= x1 || y0 >= y1)
    {
        PAINT_LOGE("Attempted to plot invalid rect drawRectFilled(%d, %d, %d, %d). Returning to avoid segfault", x0, y0,
                   x1, y1);
        return;
    }

    fillDisplayArea(x0, y0, x1 - 1, y1 - 1, col);
}

void drawRectFilledScaled(int x0, int y0, int x1, int y1, paletteColor_t col, int xTr, int yTr, int xScale, int yScale)
{
    fillDisplayArea(xTr + x0 * xScale, yTr + y0 * yScale, xTr + (x1)*xScale, yTr + (y1)*yScale, col);
}

void paintColorReplace(paintCanvas_t* canvas, paletteColor_t search, paletteColor_t replace)
{
    // super inefficient dumb color replace, maybe do iterated color fill later?
    if (canvas->buffered)
    {
        uint8_t searchHigh = 0xF0;
        uint8_t searchLow  = 0x0F;
        uint8_t replHigh = 0xF0;
        uint8_t replLow = 0x0F;
        for (uint8_t i = 0; i < PAINT_MAX_COLORS; i++)
        {
            if (canvas->palette[i] == search)
            {
                searchHigh = (i & 0x0F) << 4;
                searchLow = (i & 0x0F);
            }
            else if (canvas->palette[i] == replace)
            {
                replHigh = (i & 0x0F) << 4;
                replLow = (i & 0x0F);
            }
        }

        for (int n = 0; n < (canvas->w * canvas->h + 1) / 2; n++)
        {
            uint8_t val = canvas->buffer[n];
            if ((val & 0xF0) == searchHigh)
            {
                canvas->buffer[n] = (canvas->buffer[n] & 0x0F) | replHigh;
            }
            if ((val & 0x0F) == searchLow)
            {
                canvas->buffer[n] = (canvas->buffer[n] & 0xF0) | replLow;
            }
        }
        paintBlitCanvas(canvas);
    }
    else
    {
        for (uint8_t x = 0; x < canvas->w; x++)
        {
            for (uint8_t y = 0; y < canvas->h; y++)
            {
                if (getPxTft(canvas->x + x * canvas->xScale, canvas->y + y * canvas->yScale) == search)
                {
                    setPxScaled(x, y, replace, canvas->x, canvas->y, canvas->xScale, canvas->yScale);
                }
            }
        }
    }
}

void setPxScaled(int x, int y, paletteColor_t col, int xTr, int yTr, int xScale, int yScale)
{
    drawRectFilledScaled(x, y, x + 1, y + 1, col, xTr, yTr, xScale, yScale);
}

bool paintDrawWsgTemp(const wsg_t* wsg, pxStack_t* saveTo, uint16_t xOffset, uint16_t yOffset, colorMapFn_t colorSwap)
{
    size_t i = 0;

    for (uint16_t y = 0; y < wsg->h; y++)
    {
        for (uint16_t x = 0; x < wsg->w; x++, i++)
        {
            if (wsg->px[i] != cTransparent)
            {
                setPxTft(xOffset + x, yOffset + y,
                         colorSwap ? colorSwap(getPxTft(xOffset + x, yOffset + y)) : wsg->px[i]);
            }
        }
    }

    return true;
}

// Calculate the maximum possible [square] scale, given the display's dimensions and the image dimensions, plus any
// margins required
uint8_t paintGetMaxScale(uint16_t imgW, uint16_t imgH, uint16_t xMargin, uint16_t yMargin)
{
    // Prevent infinite loops and overflows
    if (xMargin >= TFT_WIDTH || yMargin >= TFT_WIDTH)
    {
        return 1;
    }

    if (imgW == 0 || imgH == 0)
    {
        return 0;
    }

    uint16_t maxW = TFT_WIDTH - xMargin;
    uint16_t maxH = TFT_HEIGHT - yMargin;

    uint8_t scale = 1;

    while (imgW * (scale + 1) <= maxW && imgH * (scale + 1) <= maxH)
    {
        scale++;
    }

    return scale;
}

/// @brief Writes the points of a pxStack_t into the given point_t array.
/// @param pxStack The pxStack_t to be converted
/// @param dest A pointer to an array of point_t. Must have room for at least pxStack->index entries.
// void paintConvertPickPoints(const pxStack_t* pxStack, point_t* dest)
// {
//     for (size_t i = 0; i < pxStack->index; i++)
//     {
//         dest[i].x = pxStack->data[i].x;
//         dest[i].y = pxStack->data[i].y;
//     }
// }

/// @brief Writes the points of a pxStack_t into the given point_t array, converting them to canvas coordinates
/// @param pxStack The pxStack_t to be converted
/// @param canvas The canvas whose coordinates they should be changed back to
/// @param dest A pointer to an array of point_t. Must have room for at least pxStackSize(pxStack)
void paintConvertPickPointsScaled(const pxStack_t* pxStack, paintCanvas_t* canvas, point_t* dest)
{
    for (size_t i = 0; i < pxStackSize(pxStack); i++)
    {
        dest[i].x = (pxStack->data[i].x - canvas->x) / canvas->xScale;
        dest[i].y = (pxStack->data[i].y - canvas->y) / canvas->yScale;
    }
}

uint16_t canvasToDispX(const paintCanvas_t* canvas, uint16_t x)
{
    return canvas->x + x * canvas->xScale;
}

uint16_t canvasToDispY(const paintCanvas_t* canvas, uint16_t y)
{
    return canvas->y + y * canvas->yScale;
}

void swap(uint8_t* a, uint8_t* b)
{
    *a ^= *b;
    *b ^= *a;
    *a ^= *b;
}
