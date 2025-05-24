#ifndef _HEATSHRINK_UTIL_H_
#define _HEATSHRINK_UTIL_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

bool writeHeatshrinkFile(uint8_t* input, uint32_t len, const char* outFilePath);
bool writeHeatshrinkFileHandle(uint8_t* input, uint32_t len, FILE* outFile);

#endif