//==============================================================================
// Includes
//==============================================================================

#include <string.h> // For memcpy()

#include <esp_heap_caps.h>

#include "heatshrink_helper.h"
#include "wsgCanvas.h"

//==============================================================================
// Functions
//==============================================================================

void canvasBlankInit(wsg_t* canvas, int width, int height, paletteColor_t startColor, bool spiRam)
{
    canvas->h  = height;
    canvas->w  = width;
    canvas->px = (paletteColor_t*)heap_caps_malloc_tag(sizeof(paletteColor_t) * canvas->w * canvas->h,
                                                       spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_8BIT, "wsg");
    for (int idx = 0; idx < (height * width); idx++)
    {
        canvas->px[idx] = startColor;
    }
}

void canvasDraw(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY)
{
    wsgPalette_t pal;
    wsgPaletteReset(&pal);
    canvasDrawPalette(canvas, image, startX, startY, pal);
}

void canvasDrawPalette(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, wsgPalette_t pal)
{
    // Load the WSG from the file Idx
    uint32_t decompressedSize = 0;
    uint8_t* decompressedBuf  = readHeatshrinkFile(image, &decompressedSize, false);
    int w                     = (decompressedBuf[0] << 8) | decompressedBuf[1];
    int h                     = (decompressedBuf[2] << 8) | decompressedBuf[3];

    // Overlay the new image using the offsets
    // - Offsets can be negative to move them up or left
    // - Pixels outside the canvas are not drawn
    // - Clear pixels don't overwrite data underneath
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int idx            = (y * w) + x + 4; // 4 is the offset into the data past the dims
            int xPos           = startX + x;
            int yPos           = startY + y;
            paletteColor_t col = pal.newColors[decompressedBuf[idx]];
            if (col == cTransparent ||  // If transparent
                xPos < 0 ||             // If pixel is too far lef
                xPos > canvas->w - 1 || // If pixel is too high
                yPos < 0 ||             // If pixel is too far right
                yPos > canvas->h - 1    // If pixel is too low
            )
            {
                continue;
            }
            canvas->px[((yPos)*canvas->w) + xPos] = col; // Have to use target width for proper wrapping
        }
    }

    // Free buffered data
    heap_caps_free(decompressedBuf);
}