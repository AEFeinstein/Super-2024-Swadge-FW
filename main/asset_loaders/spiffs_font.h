#ifndef _SPIFFS_FONT_H_
#define _SPIFFS_FONT_H_

#include <stdint.h>
#include <stdbool.h>

#include "font.h"

bool loadFont(const char* name, font_t* font);
void freeFont(font_t* font);

#endif