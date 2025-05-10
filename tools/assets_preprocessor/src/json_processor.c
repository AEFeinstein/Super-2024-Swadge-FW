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

bool process_json(const char* infile, const char* outFilePath)
{
#ifdef JSON_COMPRESSION
    /* Change the file extension */
    // TODO
    // char* dotptr = strrchr(outFilePath, '.');
    // snprintf(&dotptr[1], strlen(dotptr), "hjs");
#endif

    /* Read input file */
    FILE* fp = fopen(infile, "rb");

    if (!fp)
    {
        return false;
    }
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
    if (!outFile)
    {
        return false;
    }
    fwrite(jsonInStr, sz, 1, outFile);
    fclose(outFile);
#else
    if (!writeHeatshrinkFile((uint8_t*)jsonInStr, sz, outFilePath))
    {
        return false;
    }
#endif

    return true;
}