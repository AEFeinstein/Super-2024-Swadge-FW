#include "cosCrunchUtil.h"

#include "macros.h"
#include "wsg.h"
#include "wsgPalette.h"

#include <inttypes.h>
#include <string.h>

void tintPalette(wsgPalette_t* palette, const tintColor_t* tintColor)
{
    palette->newColors[PALETTE_LOWLIGHT]  = tintColor->lowlight;
    palette->newColors[PALETTE_BASE]      = tintColor->base;
    palette->newColors[PALETTE_HIGHLIGHT] = tintColor->highlight;
    if (tintColor->baseAlt != 0)
    {
        palette->newColors[PALETTE_BASE_ALT] = tintColor->baseAlt;
    }
}

void drawToCanvas(wsg_t canvas, wsg_t wsg, uint16_t x, uint16_t y)
{
    uint16_t width  = MIN(wsg.w, canvas.w - x);
    uint16_t height = MIN(wsg.h, canvas.h - y);
    for (uint16_t wsgY = 0; wsgY < height; wsgY++)
    {
        for (uint16_t wsgX = 0; wsgX < width; wsgX++)
        {
            if (wsg.px[wsgX + wsgY * wsg.w] != cTransparent)
            {
                canvas.px[x + wsgX + (y + wsgY) * canvas.w] = wsg.px[wsgX + wsgY * wsg.w];
            }
        }
    }
}

void drawToCanvasTint(wsg_t canvas, wsg_t wsg, int32_t x, int32_t y, int32_t rotationDeg, const tintColor_t* tintColor)
{
    uint16_t width  = MIN(wsg.w, canvas.w - x);
    uint16_t height = MIN(wsg.h, canvas.h - y);
    for (uint16_t wsgY = 0; wsgY < height; wsgY++)
    {
        for (uint16_t wsgX = 0; wsgX < width; wsgX++)
        {
            if (wsg.px[wsgX + wsgY * wsg.w] != cTransparent)
            {
                paletteColor_t color = wsg.px[wsgX + wsgY * wsg.w];
                if (color == PALETTE_LOWLIGHT)
                {
                    color = tintColor->lowlight;
                }
                else if (color == PALETTE_BASE)
                {
                    color = tintColor->base;
                }
                else if (color == PALETTE_HIGHLIGHT)
                {
                    color = tintColor->highlight;
                }

                int32_t tx = wsgX, ty = wsgY;
                if (rotationDeg != 0)
                {
                    rotatePixel(&tx, &ty, rotationDeg, wsg.w, wsg.h);
                }

                canvas.px[x + wsgX + (y + wsgY) * canvas.w] = color;
            }
        }
    }
}

void drawToCanvasTile(wsg_t canvas, wsg_t wsg, uint16_t x, uint16_t y)
{
    uint16_t width  = MIN(wsg.w, canvas.w - x);
    uint16_t height = MIN(wsg.h, canvas.h - y);
    for (uint16_t wsgY = 0; wsgY < height; wsgY++)
    {
        memcpy(&canvas.px[(wsgY + y) * canvas.w + x], &wsg.px[wsgY * wsg.w], width * sizeof(paletteColor_t));
    }
}
