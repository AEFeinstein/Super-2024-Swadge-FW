#ifndef _HEATSHRINK_HELPER_H_
#define _HEATSHRINK_HELPER_H_

#include <stdint.h>
#include <stdbool.h>

#include "heatshrink_decoder.h"
#include "heatshrink_encoder.h"

uint8_t* readHeatshrinkFileInplace(const char* fname, uint32_t* outsize, uint8_t* decompressedBuf,
                                   heatshrink_decoder* hsd);
uint8_t* readHeatshrinkFile(const char* fname, uint32_t* outsize, bool readToSpiRam);
uint8_t* readHeatshrinkNvs(const char* namespace, const char* key, uint32_t* outsize, bool spiRam);
uint32_t heatshrinkCompress(uint8_t* dest, const uint8_t* src, uint32_t size);
bool writeHeatshrinkNvs(const char* namespace, const char* key, const uint8_t* data, uint32_t size);
bool heatshrinkDecompress(uint8_t* dest, uint32_t* destSize, const uint8_t* source, uint32_t sourceSize);

#endif