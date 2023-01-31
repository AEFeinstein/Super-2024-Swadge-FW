#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "txt_processor.h"
#include "heatshrink_encoder.h"
#include "fileUtils.h"

long remove_chars(char* str, long len, char c);

/**
 * @brief Removes all instances of a given char from a string. Modifies the string in-place and sets a new null
 * terminator, if needed
 *
 * @param str string to remove chars from
 * @param len number of chars in the string, including null terminator
 * @param c char to remove
 * @return long number of chars in the new string, including null terminator
 */
long remove_chars(char* str, long len, char c)
{
    char *strReadPtr = str, *strWritePtr = str;
    long newLen = 1;
    for (long i = 0; i < len && *strReadPtr; i++)
    {
        *strWritePtr = *strReadPtr++;
        newLen += (*strWritePtr != c);
        strWritePtr += (*strWritePtr != c);
    }
    *strWritePtr = '\0';

    return newLen;
}

void process_txt(const char* infile, const char* outdir)
{
    /* Determine if the output file already exists */
    char outFilePath[128] = {0};
    strcat(outFilePath, outdir);
    strcat(outFilePath, "/");
    strcat(outFilePath, get_filename(infile));

    if (doesFileExist(outFilePath))
    {
        printf("Output for %s already exists\n", infile);
        return;
    }

    /* Read input file */
    FILE* fp = fopen(infile, "rb");
    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char txtInStr[sz + 1];
    fread(txtInStr, sz, 1, fp);
    txtInStr[sz] = 0;
    long newSz   = remove_chars(txtInStr, sz, '\r');
    fclose(fp);

    /* Write input directly to output */
    FILE* outFile = fopen(outFilePath, "wb");
    fwrite(txtInStr, newSz, 1, outFile);
    fclose(outFile);
}
