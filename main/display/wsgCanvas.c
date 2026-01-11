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

#include <string.h> // For memset()

#include <esp_heap_caps.h>

#include "fs_wsg.h"
#include "wsgCanvas.h"

//==============================================================================
// Consts
//==============================================================================

// If we don't want it in memory, we need it to be const. If it's constant, we can't run the reset() function. So we
// have this.
static const wsgPalette_t defaultPalette
    = {.newColors = {
           c000, c001, c002, c003, c004, c005, c010, c011, c012, c013, c014, c015, c020,         c021, c022, c023, c024,
           c025, c030, c031, c032, c033, c034, c035, c040, c041, c042, c043, c044, c045,         c050, c051, c052, c053,
           c054, c055, c100, c101, c102, c103, c104, c105, c110, c111, c112, c113, c114,         c115, c120, c121, c122,
           c123, c124, c125, c130, c131, c132, c133, c134, c135, c140, c141, c142, c143,         c144, c145, c150, c151,
           c152, c153, c154, c155, c200, c201, c202, c203, c204, c205, c210, c211, c212,         c213, c214, c215, c220,
           c221, c222, c223, c224, c225, c230, c231, c232, c233, c234, c235, c240, c241,         c242, c243, c244, c245,
           c250, c251, c252, c253, c254, c255, c300, c301, c302, c303, c304, c305, c310,         c311, c312, c313, c314,
           c315, c320, c321, c322, c323, c324, c325, c330, c331, c332, c333, c334, c335,         c340, c341, c342, c343,
           c344, c345, c350, c351, c352, c353, c354, c355, c400, c401, c402, c403, c404,         c405, c410, c411, c412,
           c413, c414, c415, c420, c421, c422, c423, c424, c425, c430, c431, c432, c433,         c434, c435, c440, c441,
           c442, c443, c444, c445, c450, c451, c452, c453, c454, c455, c500, c501, c502,         c503, c504, c505, c510,
           c511, c512, c513, c514, c515, c520, c521, c522, c523, c524, c525, c530, c531,         c532, c533, c534, c535,
           c540, c541, c542, c543, c544, c545, c550, c551, c552, c553, c554, c555, cTransparent,
       }};

//==============================================================================
// Functions
//==============================================================================

void canvasBlankInit(wsg_t* canvas, int width, int height, paletteColor_t startColor, bool spiRam)
{
    canvas->h  = height;
    canvas->w  = width;
    canvas->px = (paletteColor_t*)heap_caps_malloc(sizeof(paletteColor_t) * canvas->w * canvas->h,
                                                   spiRam ? MALLOC_CAP_SPIRAM : MALLOC_CAP_8BIT);
    memset(canvas->px, startColor, canvas->h * canvas->w * sizeof(paletteColor_t));
}

void canvasDrawSimple(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY)
{
    canvasDrawPal(canvas, image, startX, startY, false, false, 0, NULL);
}

void canvasDrawSimplePal(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, wsgPalette_t* pal)
{
    canvasDrawPal(canvas, image, startX, startY, false, false, 0, pal);
}

void canvasDraw(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, bool flipX, bool flipY, int32_t rotateDeg)
{
    canvasDrawPal(canvas, image, startX, startY, flipX, flipY, rotateDeg, NULL);
}

void canvasDrawPal(wsg_t* canvas, cnfsFileIdx_t image, int startX, int startY, bool flipX, bool flipY,
                   int32_t rotateDeg, wsgPalette_t* pal)
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

    // If palettes are used, load the custom one. Otherwise, stick with default colors.
    const wsgPalette_t* colorShift = &defaultPalette;
    if (pal != NULL)
    {
        colorShift = pal;
    }

    // iterate through each pixel in the new WSG
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            // Get initial values
            int xPos = startX + x;
            int yPos = startY + y;
            int nX   = (flipX) ? w - x - 1 : x;
            int nY   = (flipY) ? h - y - 1 : y;
            // Set color to put onto canvas
            paletteColor_t col
                = colorShift
                      ->newColors[decompressedBuf[(nY * w) + nX + 4]]; // 4 is the offset into the data past the dims
            // Check if pixel is out of bounds or transparent, continue if so
            if (col == cTransparent ||  // If transparent
                xPos < 0 ||             // If pixel is too far lef
                xPos > canvas->w - 1 || // If pixel is too high
                yPos < 0 ||             // If pixel is too far right
                yPos > canvas->h - 1    // If pixel is too low
            )
            {
                continue;
            }

            // Handle rotation if required
            if (rotateDeg != 0) // Only enter this block if rotation isn't zero to save processing time
            {
                int32_t tx = x;
                int32_t ty = y;
                rotatePixel(&tx, &ty, rotateDeg, w, h);
                xPos = tx + startX;
                yPos = ty + startY;
                if (xPos < 0 || xPos > canvas->w - 1 || yPos < 0 || yPos > canvas->h - 1)
                {
                    continue;
                }
            }

            // Copy pixel to canvas
            canvas->px[((yPos)*canvas->w) + xPos] = col; // Have to use target width for proper wrapping
        }
    }

    // Free buffered data
    heap_caps_free(decompressedBuf);
}