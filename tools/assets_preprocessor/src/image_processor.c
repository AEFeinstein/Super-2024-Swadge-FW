#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

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

#include "assets_preprocessor.h"
#include "image_processor.h"

#include "heatshrink_encoder.h"
#include "heatshrink_util.h"

#include "fileUtils.h"

#define CLAMP(x, l, u) ((x) < l ? l : ((x) > u ? u : (x)))

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    int eR;
    int eG;
    int eB;
    bool isDrawn;
} pixel_t;

void shuffleArray(uint32_t* ar, uint32_t len);
int isNeighborNotDrawn(pixel_t** img, int x, int y, int w, int h);
void spreadError(pixel_t** img, int x, int y, int w, int h, int teR, int teG, int teB, float diagScalar);
bool process_image(processorInput_t* arg);

const assetProcessor_t imageProcessor
    = {.name = "wsg", .type = FUNCTION, .function = process_image, .inFmt = FMT_FILE_BIN, .outFmt = FMT_FILE_BIN};

/**
 * @brief Randomizes the order of the given array of ints
 *
 * @param ar The array to randomize
 * @param len The number of items in the array
 */
void shuffleArray(uint32_t* ar, uint32_t len)
{
    srand(time(NULL));
    for (int i = len - 1; i > 0; i--)
    {
        int index = rand() % (i + 1);
        int a     = ar[index];
        ar[index] = ar[i];
        ar[i]     = a;
    }
}

int isNeighborNotDrawn(pixel_t** img, int x, int y, int w, int h)
{
    if (0 <= x && x < w)
    {
        if (0 <= y && y < h)
        {
            return !(img[y][x].isDrawn) ? 1 : 0;
        }
    }
    return 0;
}

void spreadError(pixel_t** img, int x, int y, int w, int h, int teR, int teG, int teB, float diagScalar)
{
    if (0 <= x && x < w)
    {
        if (0 <= y && y < h)
        {
            if (!img[y][x].isDrawn)
            {
                img[y][x].eR = (int)(teR * diagScalar + 0.5);
                img[y][x].eG = (int)(teG * diagScalar + 0.5);
                img[y][x].eB = (int)(teB * diagScalar + 0.5);
            }
        }
    }
}

bool process_image(processorInput_t* arg)
{
    /* Load the source PNG */
    int w, h, n;
    unsigned char* data = stbi_load_from_file(arg->in.file, &w, &h, &n, 4);

    bool dither = getBoolOption(arg->options, "wsg.dither", false);

    if (NULL != data)
    {
        /* Create an array for output */
        pixel_t** image8b;
        image8b = (pixel_t**)calloc(h, sizeof(pixel_t*));
        for (int y = 0; y < h; y++)
        {
            image8b[y] = (pixel_t*)calloc(w, sizeof(pixel_t));
        }

        /* Create an array of pixel indicies, then shuffle it */
        uint32_t* indices = (uint32_t*)calloc(w * h, sizeof(uint32_t)); //[w * h];
        for (int i = 0; i < w * h; i++)
        {
            indices[i] = i;
        }
        shuffleArray(indices, w * h);

        /* For all pixels */
        for (int i = 0; i < w * h; i++)
        {
            /* Get the x, y coordinates for the random pixel */
            int x = indices[i] % w;
            int y = indices[i] / w;

            /* Get the source pixel, 8 bits per channel */
            unsigned char sourceR = data[(y * (w * 4)) + (x * 4) + 0];
            unsigned char sourceG = data[(y * (w * 4)) + (x * 4) + 1];
            unsigned char sourceB = data[(y * (w * 4)) + (x * 4) + 2];
            unsigned char sourceA = data[(y * (w * 4)) + (x * 4) + 3];

            /* Find the bit-reduced value, use rounding, 5551 for RGBA */
            image8b[y][x].r = CLAMP((127 + ((sourceR + image8b[y][x].eR) * 5)) / 255, 0, 5);
            image8b[y][x].g = CLAMP((127 + ((sourceG + image8b[y][x].eG) * 5)) / 255, 0, 5);
            image8b[y][x].b = CLAMP((127 + ((sourceB + image8b[y][x].eB) * 5)) / 255, 0, 5);
            image8b[y][x].a = (sourceA >= 128) ? 0xFF : 0x00;

            // Don't dither small sprites, it just doesn't look good
            if (dither)
            {
                /* Find the total error, 8 bits per channel */
                int teR = sourceR - ((image8b[y][x].r * 255) / 5);
                int teG = sourceG - ((image8b[y][x].g * 255) / 5);
                int teB = sourceB - ((image8b[y][x].b * 255) / 5);

                /* Count all the neighbors that haven't been drawn yet */
                int adjNeighbors = 0;
                adjNeighbors += isNeighborNotDrawn(image8b, x + 0, y + 1, w, h);
                adjNeighbors += isNeighborNotDrawn(image8b, x + 0, y - 1, w, h);
                adjNeighbors += isNeighborNotDrawn(image8b, x + 1, y + 0, w, h);
                adjNeighbors += isNeighborNotDrawn(image8b, x - 1, y + 0, w, h);
                int diagNeighbors = 0;
                diagNeighbors += isNeighborNotDrawn(image8b, x - 1, y - 1, w, h);
                diagNeighbors += isNeighborNotDrawn(image8b, x + 1, y - 1, w, h);
                diagNeighbors += isNeighborNotDrawn(image8b, x - 1, y + 1, w, h);
                diagNeighbors += isNeighborNotDrawn(image8b, x + 1, y + 1, w, h);

                /* Spread the error to all neighboring unquantized pixels, with
                 * twice as much error to the adjacent pixels as the diagonal ones
                 */
                float diagScalar = 1 / (float)((2 * adjNeighbors) + diagNeighbors);
                float adjScalar  = 2 * diagScalar;

                /* Write the error */
                spreadError(image8b, x - 1, y - 1, w, h, teR, teG, teB, diagScalar);
                spreadError(image8b, x - 1, y + 1, w, h, teR, teG, teB, diagScalar);
                spreadError(image8b, x + 1, y - 1, w, h, teR, teG, teB, diagScalar);
                spreadError(image8b, x + 1, y + 1, w, h, teR, teG, teB, diagScalar);
                spreadError(image8b, x - 1, y + 0, w, h, teR, teG, teB, adjScalar);
                spreadError(image8b, x + 1, y + 0, w, h, teR, teG, teB, adjScalar);
                spreadError(image8b, x + 0, y - 1, w, h, teR, teG, teB, adjScalar);
                spreadError(image8b, x + 0, y + 1, w, h, teR, teG, teB, adjScalar);
            }

            /* Mark the random pixel as drawn */
            image8b[y][x].isDrawn = true;
        }

        free(indices);

        /* Free stbi memory */
        stbi_image_free(data);

// #define WRITE_DITHERED_PNG
#ifdef WRITE_DITHERED_PNG
        /* Convert to a pixel buffer */
        unsigned char* pixBuf = (unsigned char*)calloc(w * h * 4, sizeof(unsigned char)); //[w*h*4];
        int pixBufIdx         = 0;
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                pixBuf[pixBufIdx++] = (image8b[y][x].r * 255) / 5;
                pixBuf[pixBufIdx++] = (image8b[y][x].g * 255) / 5;
                pixBuf[pixBufIdx++] = (image8b[y][x].b * 255) / 5;
                pixBuf[pixBufIdx++] = image8b[y][x].a;
            }
        }
        /* Write a PNG */
        char pngOutFilePath[strlen(outFilePath) + 4];
        strcpy(pngOutFilePath, outFilePath);
        strcat(pngOutFilePath, ".png");
        stbi_write_png(pngOutFilePath, w, h, 4, pixBuf, 4 * w);
        free(pixBuf);
#endif

        /* Convert to a palette buffer */
        uint32_t paletteBufSize   = sizeof(unsigned char) * w * h;
        unsigned char* paletteBuf = calloc(1, paletteBufSize);
        int paletteBufIdx         = 0;
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                if (image8b[y][x].a)
                {
                    /* Index math! The palette indices increase blue, then green, then red.
                     * Each has a value 0-5 (six levels)
                     */
                    paletteBuf[paletteBufIdx++]
                        = (image8b[y][x].b) + (6 * (image8b[y][x].g)) + (36 * (image8b[y][x].r));
                }
                else
                {
                    /* This invalid value means 'transparent' */
                    paletteBuf[paletteBufIdx++] = 6 * 6 * 6;
                }
            }
        }

        /* Free dithering memory */
        for (int y = 0; y < h; y++)
        {
            free(image8b[y]);
        }
        free(image8b);

        /* Combine the header and image*/
        uint32_t hdrAndImgSz = sizeof(uint8_t) * (4 + paletteBufSize);
        uint8_t* hdrAndImg   = calloc(1, hdrAndImgSz);
        hdrAndImg[0]         = HI_BYTE(w);
        hdrAndImg[1]         = LO_BYTE(w);
        hdrAndImg[2]         = HI_BYTE(h);
        hdrAndImg[3]         = LO_BYTE(h);
        memcpy(&hdrAndImg[4], paletteBuf, paletteBufSize);
        /* Write the compressed file */

        bool result = writeHeatshrinkFileHandle(hdrAndImg, hdrAndImgSz, arg->out.file);
        /* Cleanup */
        free(hdrAndImg);
        free(paletteBuf);

        return result;
    }

    return false;
}
