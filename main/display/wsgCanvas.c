/**
 * @file wsgCanvas.c
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Provides a canvas to paint with low memory requirements
 * @version 1.0
 * @date 2025-09-02
 *
 * @copyright Copyright (c) 2025
 *
 */

//==============================================================================
// Includes
//==============================================================================

#include <string.h> // For memcpy()

#include <esp_heap_caps.h>

#include "fs_wsg.h"
#include "wsgCanvas.h"

//==============================================================================
// Functions
//==============================================================================

void canvasBlankInit(wsg_t* canvas, int width, int height, paletteColor_t startColor, bool spiRam)
{
    canvas->h = height;
    canvas->w = width;
    canvas->px = (paletteColor_t*)heap_caps_malloc(sizeof(paletteColor_t) * canvas->w * canvas->h,
                                                   spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_8BIT);
    for (int idx = 0; idx < (height * width); idx++)
    {
        canvas->px[idx] = startColor;
    }
}

void canvasDrawSimple(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY)
{
    wsgPalette_t pal;
    wsgPaletteReset(&pal);
    canvasDrawPal(canvas, image, startX, startY, false, false, 0, pal);
}

void canvasDrawSimplePal(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, wsgPalette_t pal)
{
    canvasDrawPal(canvas, image, startX, startY, false, false, 0, pal);
}

void canvasDraw(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, bool flipX, bool flipY, int32_t rotateDeg)
{
    wsgPalette_t pal;
    wsgPaletteReset(&pal);
    canvasDrawPal(canvas, image, startX, startY, flipX, flipY, rotateDeg, pal);
}

void canvasDrawPal(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, bool flipX, bool flipY,
                   int32_t rotateDeg, wsgPalette_t pal)
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
    // - Flip over x or y axis as required
    // - Rotate pixels if necessary
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            // Check if color/position is not valid and continue
            int xPos = startX + x;
            int yPos = startY + y;
            int nX   = (flipX) ? w - x - 1 : x;
            int nY   = (flipY) ? h - y - 1 : y;
            paletteColor_t col
                = pal.newColors[decompressedBuf[(nY * w) + nX + 4]]; // 4 is the offset into the data past the dims
            if (col == cTransparent ||                               // If transparent
                xPos < 0 ||                                          // If pixel is too far lef
                xPos > canvas->w - 1 ||                              // If pixel is too high
                yPos < 0 ||                                          // If pixel is too far right
                yPos > canvas->h - 1                                 // If pixel is too low
            )
                continue;

            // Handle rotation
            if (rotateDeg != 0) // Only enter this block if rotation isn't zero to save processing time
            {
                int32_t tx = x;
                int32_t ty = y;
                rotatePixel(&tx, &ty, rotateDeg, w, h);
                xPos = tx + startX;
                yPos = ty + startY;
                if (xPos < 0 || xPos > canvas->w - 1 || yPos < 0 || yPos > canvas->h - 1)
                    continue;
            }

            // Copy pixel to canvas
            canvas->px[((yPos)*canvas->w) + xPos] = col; // Have to use target width for proper wrapping
        }
    }

    // Free buffered data
    heap_caps_free(decompressedBuf);
}