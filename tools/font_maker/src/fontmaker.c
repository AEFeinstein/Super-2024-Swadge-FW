//==============================================================================
// Includes
//==============================================================================

#include <stdbool.h>
#include <stdio.h>
#include "schrift.h"
#include "utf8_to_utf32.h"

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic push
#endif
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#if defined(__clang__) || (defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5))))
    #pragma GCC diagnostic pop
#endif

//==============================================================================
// Defines
//==============================================================================

/// The threshold between white and black pixels
#define PIXEL_THRESHOLD 0x80

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    SFT_Image img;       ///< The rendered image
    int32_t yOff;        ///< The Y offset to draw this glyph at
    int32_t minX;        ///< Where the glyph actually starts in img
    int32_t maxX;        ///< Where the glyph actually ends in img
    int32_t actualWidth; ///< The actual height of the glyph
} glyph_t;

//==============================================================================
// Function Prototypes
//==============================================================================

void renderGlyphToMem(SFT* sft, uint32_t codepoint, int32_t spaceWidth, glyph_t* glyph);
int actualCharWidth(SFT_Image* img, int32_t* minX, int32_t* maxX);
void printUsageExit(const char* format, ...);

//==============================================================================
// Variables
//==============================================================================

const char* progName = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Print a message and the program usage, then exit
 *
 * @param format A printf style format string
 * @param ... Arguments for the printf style format string
 */
void printUsageExit(const char* format, ...)
{
    // Print the message
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    // Print usage
    fprintf(stderr, "\nUsage: %s [Font file] [Font size, px]\n", progName);

    // Exit
    exit(1);
}

/**
 * @brief Render the given TTF font file into a PNG with all ASCII characters in a line, underlined
 *
 * @param argc The number of arguments, should be three
 * @param argv The argument values, [program name, font file, font size]
 * @return 0 for success, 1 for any error
 */
int main(int32_t argc, char** argv)
{
    // Save the program name
    progName = argv[0];

    // Validate arg count
    if (3 != argc)
    {
        printUsageExit("Wrong argument count");
    }

    // Pick out the arguments
    char* fontFile     = argv[1];
    float fontSizePx = atof(argv[2]);

    // Validate size
    if (0 == fontSizePx)
    {
        printUsageExit("Invalid size");
    }

    // Load the font
    SFT sft = {
        .xScale = fontSizePx,
        .yScale = fontSizePx,
        .flags  = SFT_DOWNWARD_Y,
    };
    sft.font = sft_loadfile(fontFile);

    // Validate the font was loaded
    if (NULL == sft.font)
    {
        printUsageExit("Couldn't load font (%s)", fontFile);
    }

    // Create a string with every printable ASCII character, null terminated
    char text['~' - ' ' + 2] = {0};
    for (char c = ' '; c <= '~'; c++)
    {
        text[c - ' '] = c;
    }

    // Convert ASCII characters to unicode codepoints
    uint32_t codepoints[sizeof(text)];
    int32_t n = utf8_to_utf32((unsigned char*)text, codepoints, sizeof(text)); // (const uint8_t *)

    // Declarare variable for measurement
    int32_t totalWidth = 0;
    int32_t avgWidth   = 0;
    int32_t minStartY  = 0;
    int32_t maxEndY    = 0;
    int32_t spaceWidth = 0;

    // Iterate over all characters and measure them
    for (int32_t i = 0; i < n; i++)
    {
        // Render the glyph to memory
        glyph_t glyph;
        renderGlyphToMem(&sft, codepoints[i], -1, &glyph);

        // Save the width of the space character, to be adjusted later
        if (' ' == text[i])
        {
            spaceWidth = glyph.img.width;
        }

        // Find the total width for all characters for the output image
        totalWidth += (glyph.actualWidth + 1);
        // Keep a running total width to find the average width
        avgWidth += glyph.actualWidth;

        // Find upper and lower Y bounds
        if (-glyph.yOff < minStartY)
        {
            minStartY = -glyph.yOff;
        }
        if ((-glyph.yOff + glyph.img.height) > maxEndY)
        {
            maxEndY = (-glyph.yOff + glyph.img.height);
        }

        // Cleanup
        free(glyph.img.pixels);
    }

    // Find the average width of a character, used for ' '
    avgWidth /= n;

    // Adjust width for the space character
    totalWidth += (avgWidth - spaceWidth);

    // Find the height for the output image, the characters and two pixels for underlining
    int32_t maxHeight = maxEndY - minStartY + 2;

    // Track an index to draw to
    int32_t imgXoff = 0;

    // Allocate a pixel buffer for the output PNG
    unsigned char* pixBuf = (unsigned char*)malloc(sizeof(unsigned char) * totalWidth * maxHeight * 4);
    // Set every pixel to white
    memset(pixBuf, 0xFF, sizeof(unsigned char) * totalWidth * maxHeight * 4);

    // Render each glyph to the image
    for (int32_t i = 0; i < n; i++)
    {
        // Render the glyph
        glyph_t glyph;
        renderGlyphToMem(&sft, codepoints[i], (' ' == text[i]) ? avgWidth : -1, &glyph);

        // Draw the underline for this glyph to the PNG
        for (int32_t x = 0; x < glyph.actualWidth; x++)
        {
            int32_t pngY = maxHeight - 1;
            int32_t pngX = imgXoff + x;

            pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 0] = 0x00;
            pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 1] = 0x00;
            pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 2] = 0x00;
            pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 3] = 0xFF;
        }

        // Draw the glyph to the PNG
        for (int32_t y = 0; y < glyph.img.height; y++)
        {
            int32_t pngY = (-minStartY) - glyph.yOff + y;
            for (int32_t x = glyph.minX; x <= glyph.maxX; x++)
            {
                int32_t pngX = imgXoff + x - glyph.minX;
                if (((uint8_t*)glyph.img.pixels)[y * glyph.img.width + x] >= PIXEL_THRESHOLD)
                {
                    pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 0] = 0x00;
                    pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 1] = 0x00;
                    pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 2] = 0x00;
                    pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 3] = 0xFF;
                }
                else
                {
                    pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 0] = 0xFF;
                    pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 1] = 0xFF;
                    pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 2] = 0xFF;
                    pixBuf[(pngY * totalWidth * 4) + (pngX * 4) + 3] = 0xFF;
                }
            }
        }
        imgXoff += (glyph.actualWidth + 1);

        // Cleanup
        free(glyph.img.pixels);
    }

    // Modify the input filename to be the output filename
    char outfileName[strlen(fontFile) + 8];
    outfileName[0] = 0;
    strcpy(outfileName, fontFile);
    char* dotptr = strrchr(outfileName, '.');
    sprintf(&dotptr[1], "font.png");

    // Write a PNG
    stbi_write_png(outfileName, totalWidth, maxHeight, 4, pixBuf, 4 * totalWidth);
    free(pixBuf);

    sft_freefont(sft.font);
    return 0;
}

/**
 * @brief Render a glyph to RAM
 *
 * @param sft The font to render with
 * @param codepoint The unicode codepoint to render
 * @param spaceWidth The width of a space character, or -1 if unused
 * @param glyph The glyph to render to in RAM
 */
void renderGlyphToMem(SFT* sft, uint32_t codepoint, int32_t spaceWidth, glyph_t* glyph)
{
    // Lookup the SFT glyph
    SFT_Glyph gid;
    if (sft_lookup(sft, codepoint, &gid) < 0)
    {
        printUsageExit("Glyph missing (%04X)", codepoint);
    }

    // Get metrics for the glyph
    SFT_GMetrics mtx;
    if (sft_gmetrics(sft, gid, &mtx) < 0)
    {
        printUsageExit("Bad glyph metrics (%04X)", codepoint);
    }

    // Adjust width if this is the space char
    if (spaceWidth > 0)
    {
        mtx.minWidth = spaceWidth;
    }

    // Initialize the img in the glyph
    glyph->img.width  = mtx.minWidth;
    glyph->img.height = mtx.minHeight;
    glyph->img.pixels = malloc(sizeof(char) * glyph->img.width * glyph->img.height);
    glyph->yOff       = -mtx.yOffset;

    // Render the glyph locally
    if (sft_render(sft, gid, glyph->img) < 0)
    {
        free(glyph->img.pixels);
        printUsageExit("Not rendered (%04X)", codepoint);
    }

    // Find the actual width of the rendered character
    glyph->actualWidth = actualCharWidth(&glyph->img, &glyph->minX, &glyph->maxX);

    // Adjust width if this is the space char
    if (spaceWidth > 0)
    {
        glyph->actualWidth = spaceWidth;
        glyph->minX        = 0;
        glyph->maxX        = spaceWidth;
    }
}

/**
 * @brief Find the actual width of a glyph in a rendered image
 *
 * @param img The rendered image
 * @param minX Output, where to write the X coordinate where glyph begins
 * @param maxX Output, where to write the X coordinate where glyph ends
 * @return The actual width of the glyph in the image
 */
int actualCharWidth(SFT_Image* img, int32_t* minX, int32_t* maxX)
{
    // Make aure there is something to measure
    if (img->width == 0 || img->height == 0)
    {
        *minX = 0;
        *maxX = 0;
        return img->width;
    }

    // Start with really big and small values
    *minX = 100000;
    *maxX = 0;

    // Iterate over the entire image
    for (int32_t y = 0; y < img->height; y++)
    {
        for (int32_t x = 0; x < img->width; x++)
        {
            // If this pixel is drawn
            if (((uint8_t*)img->pixels)[(y * img->width) + x] > PIXEL_THRESHOLD)
            {
                // Check and set the bounds
                if (x < *minX)
                {
                    *minX = x;
                }
                if (x > *maxX)
                {
                    *maxX = x;
                }
            }
        }
    }

    // No pixels set
    if(100000 == *minX)
    {
        *minX = 0;
    }

    // Return the actual width
    return *maxX - *minX + 1;
}
