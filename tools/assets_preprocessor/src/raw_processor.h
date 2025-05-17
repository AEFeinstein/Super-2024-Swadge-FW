#pragma once

#include <stdbool.h>
#include "assets_preprocessor.h"

bool process_raw(processorInput_t* arg);

/**
 * @brief The heatshrink processor compresses its input file with heatshrink.
 * Any files processed can be loaded from the Swadge with \ref readHeatshrinkFile()
 */
extern const assetProcessor_t heatshrinkProcessor;
