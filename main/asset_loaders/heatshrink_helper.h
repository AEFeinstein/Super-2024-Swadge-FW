#ifndef _HEATSHRINK_HELPER_H_
#define _HEATSHRINK_HELPER_H_

#include <stdint.h>
#include <stdbool.h>
#include "heatshrink_decoder.h"

uint8_t* readHeatshrinkFile(const char* fname, uint32_t* outsize, bool readToSpiRam);
uint8_t* readHeatshrinkNvs(const char* namespace, const char* key, uint32_t* outsize, bool spiRam);
uint32_t heatshrinkCompress(uint8_t* dest, const uint8_t* src, uint32_t size);
uint32_t heatshrinkDecompressBuf(heatshrink_decoder* hsd, const uint8_t* inBuf, uint32_t inSize, uint8_t* outBuf,
                                 uint32_t outsize);
bool writeHeatshrinkNvs(const char* namespace, const char* key, const uint8_t* data, uint32_t size);

#endif