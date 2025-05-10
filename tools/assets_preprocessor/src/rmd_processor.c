#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rmd_processor.h"
#include "heatshrink_encoder.h"
#include "fileUtils.h"
#include "heatshrink_util.h"

bool process_rmd(const char* infile, const char* outFilePath)
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
    char rmdInStr[sz + 1];
    fread(rmdInStr, sz, 1, fp);
    rmdInStr[sz] = 0;
    fclose(fp);

    return writeHeatshrinkFile((uint8_t*)rmdInStr, sz, outFilePath);
}
