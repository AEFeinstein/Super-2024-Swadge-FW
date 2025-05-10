#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileUtils.h"

#include "bin_processor.h"

bool process_bin(const char* infile, const char* outFilePath)
{
    /* Read input file */
    FILE* fp = fopen(infile, "rb");

    if (!fp)
    {
        return false;
    }

    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char byteString[sz + 1];
    fread(byteString, sz, 1, fp);
    byteString[sz] = 0;
    fclose(fp);

    /* Write input directly to output */
    FILE* outFile = fopen(outFilePath, "wb");

    if (!outFile)
    {
        return false;
    }

    fwrite(byteString, sz, 1, outFile);
    fclose(outFile);

    return true;
}