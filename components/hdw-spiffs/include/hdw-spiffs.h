#ifndef _HDW_SPIFFS_H_
#define _HDW_SPIFFS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "spiffs_json.h"
#include "spiffs_txt.h"
#include "spiffs_font.h"
#include "spiffs_wsg.h"
#include "spiffs_sng.h"

bool initSpiffs(void);
bool deinitSpiffs(void);

uint8_t* spiffsReadFile(const char* fname, size_t* outsize, bool readToSpiRam);
uint8_t* readHeatshrinkFile(const char* fname, uint32_t* outsize, bool readToSpiRam);

#endif
