#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include "heatshrink_util.h"

#include "raw_processor.h"

bool process_heatshrink(processorInput_t* arg);

const assetProcessor_t heatshrinkProcessor = {
    .name     = "heatshrink",
    .type     = FUNCTION,
    .function = process_heatshrink,
    .inFmt    = FMT_DATA,
    .outFmt   = FMT_FILE_BIN,
};

bool process_heatshrink(processorInput_t* arg)
{
    // Write the compressed bytes to a file
    return writeHeatshrinkFileHandle(arg->in.data, arg->in.length, arg->out.file);
}