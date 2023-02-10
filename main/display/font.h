#ifndef _FONT_H_
#define _FONT_H_

#include <stdint.h>

#include "palette.h"
#include "spiffs_font.h"

void drawChar(paletteColor_t color, int h, const font_ch_t* ch, int16_t xOff, int16_t yOff);
int16_t drawText(const font_t* font, paletteColor_t color, const char* text, int16_t xOff, int16_t yOff);
const char* drawTextWordWrap(const font_t* font, paletteColor_t color, const char* text, int16_t* xOff, int16_t* yOff,
                             int16_t xMax, int16_t yMax);
uint16_t textWidth(const font_t* font, const char* text);
uint16_t textHeight(const font_t* font, const char* text, int16_t width, int16_t maxHeight);

#endif