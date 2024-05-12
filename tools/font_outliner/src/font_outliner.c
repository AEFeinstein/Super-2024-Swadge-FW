//==============================================================================
// Includes
//==============================================================================

#include <stdbool.h>
#include <stdio.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

//==============================================================================
// Defines
//==============================================================================

/// The threshold between white and black pixels
#define PIXEL_THRESHOLD 0x80

//==============================================================================
// Function Prototypes
//==============================================================================

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
    fprintf(stderr, "\nUsage: %s [Font file]\n", progName);

    // Exit
    exit(1);
}

bool isBlack(unsigned char* data, int w, int h, int x, int y)
{
    if(x < 0 || x >= w || y < 0 || y >= h)
    {
        return false;
    }

    x *= 4;
    w *= 4;
    int rIdx = (y * w) + x + 0;
    int gIdx = (y * w) + x + 1;
    int bIdx = (y * w) + x + 2;
    int aIdx = (y * w) + x + 3;

    return (data[rIdx] < PIXEL_THRESHOLD) &&(data[gIdx] < PIXEL_THRESHOLD) &&(data[bIdx] < PIXEL_THRESHOLD);
}

void setColor(unsigned char* data, int w, int x, int y, bool isBlack)
{
    x *= 4;
    w *= 4;
    data[(y * w) + x + 0] = isBlack ? 0x00 : 0xFF;
    data[(y * w) + x + 1] = isBlack ? 0x00 : 0xFF;
    data[(y * w) + x + 2] = isBlack ? 0x00 : 0xFF;
    data[(y * w) + x + 3] = 0xFF;
}

/**
 * @brief Edge-detect a black-and-white font file
 *
 * @param argc The number of arguments, should be two
 * @param argv The argument values, [program name, font file]
 * @return 0 for success, 1 for any error
 */
int main(int32_t argc, char** argv)
{
    // Save the program name
    progName = argv[0];

    // Validate arg count
    if (2 != argc)
    {
        printUsageExit("Wrong argument count");
    }

    // Pick out the arguments
    char* fontFile     = argv[1];

    // Load the font
    int w, h, n;
    unsigned char* data = stbi_load(fontFile, &w, &h, &n, 4);
    unsigned char* outlineData = malloc(sizeof(unsigned char) * w * h * 4);
    memcpy(outlineData, data, sizeof(unsigned char) * w * h * 4);

    // Validate the font was loaded
    if (NULL == data)
    {
        printUsageExit("Couldn't load font (%s)", fontFile);
    }

    // For each pixel
    for(int y = 0; y < h - 1; y++)
    {
        for(int x = 0; x < w; x++)
        {
            // If it's black
            if(isBlack(data, w, h, x, y))
            {
                // And it has a white neighbor
                if(!isBlack(data, w, h - 1, x - 1, y) || 
                   !isBlack(data, w, h - 1, x + 1, y) || 
                   !isBlack(data, w, h - 1, x, y - 1) || 
                   !isBlack(data, w, h - 1, x, y + 1))
                {
                    setColor(outlineData, w, x, y, true);                
                }
                else
                {
                    setColor(outlineData, w, x, y, false);
                }
            }
            else
            {
                // Write a white pixel
                setColor(outlineData, w, x, y, false);                
            }
        }
    }

    // Modify the input filename to be the output filename
    char outfileName[strlen(fontFile) + 8];
    outfileName[0] = 0;
    strcpy(outfileName, fontFile);
    char* dotptr = strrchr(outfileName, '.');
    sprintf(&dotptr[1], "outline.png");

    // Write a PNG
    stbi_write_png(outfileName, w, h, 4, outlineData, 4 * w);
    free(outlineData);
    stbi_image_free(data);

    return 0;
}
