#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "fileUtils.h"
#include "bin_processor.h"
#include "assets_preprocessor.h"

bool process_bin(processorInput_t* in);

const assetProcessor_t binProcessor =
{
    .inExt = "bin",
    .outExt = "bin",
    .inFmt = FMT_DATA,
    .outFmt = FMT_DATA,
    .type = FUNCTION,
    .function = process_bin
};

bool process_bin(processorInput_t* data)
{
    // this one's _REAL_ simple now
    data->out.data = data->in.data;
    return true;
}
