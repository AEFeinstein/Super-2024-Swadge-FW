#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "stb_image.h"
#include "assets_preprocessor.h"
#include "font_processor.h"
#include "fileUtils.h"

bool process_font(processorInput_t* arg);

const assetProcessor_t fontProcessor = {
    .type = FUNCTION,
    .function = process_font,
    .inFmt = FMT_FILE_BIN,
    .outFmt = FMT_FILE_BIN,
};

uint32_t getPx(unsigned char* data, int w, int x, int y);
void appendCharToFile(FILE* fp, unsigned char* data, int w, int h, int charStartX, int charEndX);

/**
 * TODO
 *
 * @param data
 * @param w
 * @param x
 * @param y
 * @return uint32_t
 */
uint32_t getPx(unsigned char* data, int w, int x, int y)
{
    return (data[(y * w * 4) + (x * 4) + 0] << 0) | (data[(y * w * 4) + (x * 4) + 1] << 8)
           | (data[(y * w * 4) + (x * 4) + 2] << 16) | (data[(y * w * 4) + (x * 4) + 3] << 24);
}

/**
 * @brief TODO
 *
 * @param fp
 * @param data
 * @param w
 * @param h
 * @param charStartX
 * @param charEndX
 */
void appendCharToFile(FILE* fp, unsigned char* data, int w, int h, int charStartX, int charEndX)
{
    /* Write the output width */
    putc(charEndX - charStartX + 1, fp);

    /* Write the output bitmap data */
    unsigned char outByte = 0;
    uint8_t bitIdx        = 0;
    for (int chy = 0; chy < h - 2; chy++)
    {
        for (int chx = charStartX; chx <= charEndX; chx++)
        {
            if (0 == (0xFFFFFF & getPx(data, w, chx, chy)))
            {
                outByte |= (1 << bitIdx);
            }

            bitIdx++;
            if (8 == bitIdx)
            {
                putc(outByte, fp);
                outByte = 0;
                bitIdx  = 0;
            }
        }
    }

    /* Write any straggling bits */
    if (0 != bitIdx)
    {
        putc(outByte, fp);
        outByte = 0;
        bitIdx  = 0;
    }
}

/**
 * @brief TODO
 *
 * @param infile
 * @param outdir
 */
bool process_font(processorInput_t* arg)
{
    FILE* fp = arg->out.file; // fopen(outFilePath, "wb+");

    /* Load the font PNG */
    int w, h, n;
    unsigned char* data = stbi_load_from_file(arg->in.file, &w, &h, &n, 4);

    if (!data)
    {
        fprintf(stderr, "ERR: Could not load font with stbi_load_from_file(%s, ...)", arg->inFilename);
        return false;
    }

    int charsWritten = 0;

    /* Write the output height, excluding spacing row */
    putc(h - 2, fp);

    /* Start scanning the PNG for charcters */
    int charStartX      = 0;
    int charEndX        = 0;
    bool isCountingChar = false;
    char ch             = ' ';
    for (int x = 0; x < w; x++)
    {
        switch (0xFFFFFF & getPx(data, w, x, h - 1))
        {
            case 0:
            {
                /* black px */
                charEndX       = x;
                isCountingChar = true;
                break;
            }
            default:
            {
                /* white px (not black) */
                if (isCountingChar)
                {
                    appendCharToFile(fp, data, w, h, charStartX, charEndX);
                    charsWritten++;
                    /* Increment the char (dbg) */
                    ch++;
                }
                charStartX     = x + 1;
                charEndX       = x + 1;
                isCountingChar = false;
                break;
            }
        }
    }

    /* Check for leftovers */
    if (charStartX != charEndX)
    {
        appendCharToFile(fp, data, w, h, charStartX, charEndX);
        charsWritten++;
    }

    /* Error check */
    if (95 != charsWritten && 96 != charsWritten)
    {
        fprintf(stderr, "ERROR: font %s isnt 95 or 96 chars (%d chars)\n", arg->inFilename, charsWritten);
    }

    /* Cleanup */
    stbi_image_free(data);

    return true;
}
