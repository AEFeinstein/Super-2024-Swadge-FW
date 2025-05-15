#include <stdbool.h>
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

bool process_json(processorInput_t* arg);

const assetProcessor_t jsonProcessor = {
    .type = FUNCTION,
    .function = process_json,
    .inFmt = FMT_TEXT,
    .outFmt = FMT_FILE_BIN,
};

bool process_json(processorInput_t* arg)
{
    // Parse the JSON to make sure it's valid
    cJSON* json = cJSON_Parse(arg->in.text);
    if (!json)
    {
        fprintf(stderr, "[ERR] Invalid JSON in %s\n", arg->inFilename);
        return false;
    }
    // Cool, it's valid, don't need it anymore!
    cJSON_Delete(json);

#ifdef JSON_COMPRESSION
    /* Change the file extension */
    // TODO
    // char* dotptr = strrchr(outFilePath, '.');
    // snprintf(&dotptr[1], strlen(dotptr), "hjs");
#endif

#ifndef JSON_COMPRESSION
    /* Write input directly to output */
    fwrite(arg->in.text, arg->in.textSize - 1, 1, arg->out.file);
#else
    if (!writeHeatshrinkFileHandle((uint8_t*)arg->in.text, arg->in.textSize - 1, arg->out.file))
    {
        return false;
    }
#endif

    return true;
}