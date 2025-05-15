#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rmd_processor.h"
#include "heatshrink_encoder.h"
#include "fileUtils.h"
#include "heatshrink_util.h"

bool process_rmd(processorInput_t* arg);

const assetProcessor_t rmdProcessor = {
    .type = FUNCTION,
    .function = process_rmd,
    .inFmt = FMT_TEXT,
    .outFmt = FMT_FILE_BIN,
};

bool process_rmd(processorInput_t* arg)
{
    return writeHeatshrinkFileHandle((uint8_t*)arg->in.text, arg->in.textSize, arg->out.file);
}
