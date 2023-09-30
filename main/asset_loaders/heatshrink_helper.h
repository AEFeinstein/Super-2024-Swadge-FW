#ifndef _HEATSHRINK_HELPER_H_
#define _HEATSHRINK_HELPER_H_

#include <stdint.h>
#include <stdbool.h>

uint8_t* readHeatshrinkFile(const char* fname, uint32_t* outsize, bool readToSpiRam);
uint8_t* readHeatshrinkNvs(const char* namespace, const char* key, uint32_t* outsize, bool spiRam);
bool writeHeatshrinkNvs(const char* namespace, const char* key, const uint8_t* data, uint32_t size);

#endif