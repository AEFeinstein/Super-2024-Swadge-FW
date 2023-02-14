#ifndef _SPIFFS_FONT_H_
#define _SPIFFS_FONT_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint8_t w;
    uint8_t* bitmap;
} font_ch_t;

typedef struct
{
    uint8_t h;
    font_ch_t chars['~' - ' ' + 2]; // enough space for all printed ascii chars, and pi
} font_t;

bool loadFont(const char* name, font_t* font);
void freeFont(font_t* font);

#endif