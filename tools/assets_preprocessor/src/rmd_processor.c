#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rmd_processor.h"
#include "heatshrink_encoder.h"
#include "fileUtils.h"
#include "heatshrink_util.h"

void process_rmd(const char* infile, const char* outdir)
{
    /* Determine if the output file already exists */
    char outFilePath[128] = {0};
    strcat(outFilePath, outdir);
    strcat(outFilePath, "/");
    strcat(outFilePath, get_filename(infile));

    /* Change the file extension */
    char* dotptr = strrchr(outFilePath, '.');
    snprintf(&dotptr[1], strlen(dotptr), "rmh");

    if(!isSourceFileNewer(infile, outFilePath))
    {
        return;
    }
    else if (doesFileExist(outFilePath))
    {
        printf("[assets-preprocessor] %s modified! Regenerating %s\n", infile, get_filename(outFilePath));
    }

    /* Read input file */
    FILE* fp = fopen(infile, "rb");
    fseek(fp, 0L, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    char rmdInStr[sz + 1];
    fread(rmdInStr, sz, 1, fp);
    rmdInStr[sz] = 0;
    fclose(fp);

    writeHeatshrinkFile((uint8_t*)rmdInStr, sz, outFilePath);
}