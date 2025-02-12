#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "json_processor.h"
#include "cJSON.h"
#include "heatshrink_encoder.h"
#include "fileUtils.h"
#include "heatshrink_util.h"

#define JSON_COMPRESSION

void process_json(const char* infile, const char* outdir)
{
    /* Determine if the output file already exists */
    char outFilePath[128] = {0};
    strcat(outFilePath, outdir);
    strcat(outFilePath, "/");
    strcat(outFilePath, get_filename(infile));

#ifdef JSON_COMPRESSION
    /* Change the file extension */
    char* dotptr = strrchr(outFilePath, '.');
    snprintf(&dotptr[1], strlen(dotptr), "hjs");
#endif

    if (!isSourceFileNewer(infile, outFilePath))
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
    char jsonInStr[sz + 1];
    fread(jsonInStr, sz, 1, fp);
    jsonInStr[sz] = 0;
    fclose(fp);

#ifndef JSON_COMPRESSION
    /* Write input directly to output */
    FILE* outFile = fopen(outFilePath, "wb");
    fwrite(jsonInStr, sz, 1, outFile);
    fclose(outFile);
#else
    writeHeatshrinkFile((uint8_t*)jsonInStr, sz, outFilePath);
#endif
}