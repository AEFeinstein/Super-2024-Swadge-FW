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

bool process_json(processorInput_t* arg);

const assetProcessor_t jsonProcessor = {
    .name     = "json",
    .type     = FUNCTION,
    .function = process_json,
    .inFmt    = FMT_TEXT,
    .outFmt   = FMT_FILE_BIN,
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

    // cJSON says to allocate 5 more bytes than we actually need, so...
    char textBuf[arg->in.textSize + 10];
    char* jsonText = textBuf;
    if (!cJSON_PrintPreallocated(json, textBuf, sizeof(textBuf), false))
    {
        fprintf(stderr, "[WRN] Unable to print JSON text; using original text\n");
        jsonText = arg->in.text;
    }

    // Cool, it's valid, don't need it anymore!
    cJSON_Delete(json);

    // TODO should we have a way to change the output extension in general?
    /* Change the file extension */
    // char* dotptr = strrchr(outFilePath, '.');
    // snprintf(&dotptr[1], strlen(dotptr), "hjs");

    bool compress = getBoolOption(arg->options, "json.compress", true);

    if (compress)
    {
        return writeHeatshrinkFileHandle((uint8_t*)jsonText, strlen(jsonText), arg->out.file);
    }
    else
    {
        return 0 != fwrite(arg->in.text, arg->in.textSize - 1, 1, arg->out.file);
    }
}