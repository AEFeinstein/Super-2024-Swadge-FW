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

void canvasDraw(wsg_t* canvas, cnfsFileIdx_t splat, int startX, int startY)
{
    // Load the WSG from the file Idx
    uint32_t decompressedSize = 0;
    uint8_t* decompressedBuf  = readHeatshrinkFile(splat, &decompressedSize, false);
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
            if(decompressedBuf[(y * w) + x + 4] == cTransparent)
            {
                continue;
            }
            canvas->px[(y * canvas->w) + x] = decompressedBuf[(y * w) + x + 4];
        }
    }

    // memcpy(canvas->px, &decompressedBuf[4], decompressedSize - 4);

    // Free buffered data
    heap_caps_free(decompressedBuf);
}