#ifndef _HEATSHRINK_HELPER_H_
#define _HEATSHRINK_HELPER_H_

#include <stdint.h>
#include <stdbool.h>

uint8_t* readHeatshrinkFile(const char* fname, uint32_t* outsize, bool readToSpiRam);

#endif