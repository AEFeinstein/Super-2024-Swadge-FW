#ifndef _TXT_PROCESSOR_H_
#define _TXT_PROCESSOR_H_

#include "assets_preprocessor.h"

/**
 * @brief The text processor strips out any carriage returns (`\r`) or non-ASCII characters from the input file and saves it uncompressed
 * 
 */
extern const assetProcessor_t textProcessor;

#endif