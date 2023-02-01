#ifndef _HDW_SPIFFS_H_
#define _HDW_SPIFFS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "spiffs_json.h"
#include "spiffs_txt.h"
#include "spiffs_font.h"
#include "spiffs_wsg.h"

bool initSpiffs(void);
bool deinitSpiffs(void);

bool spiffsReadFile(const char* fname, uint8_t** output, size_t* outsize, bool readToSpiRam);

#endif
