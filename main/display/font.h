/*! \file font.h
 *
 * \section font_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section font_usage Usage
 *
 * TODO doxygen
 *
 * \section font_example Example
 *
 * \code{.c}
 * TODO doxygen
 * \endcode
 */

#ifndef _FONT_H_
#define _FONT_H_

#include <stdint.h>

#include "palette.h"

typedef struct
{
    uint8_t w;       ///< The width of this character
    uint8_t* bitmap; ///< This character's bitmap data
} font_ch_t;

typedef struct
{
    uint8_t h;                      ///< The height of this font. All chars have the same height
    font_ch_t chars['~' - ' ' + 2]; ///< An array of characters, enough space for all printed ASCII chars, and pi
} font_t;

void drawChar(paletteColor_t color, int h, const font_ch_t* ch, int16_t xOff, int16_t yOff);
int16_t drawText(const font_t* font, paletteColor_t color, const char* text, int16_t xOff, int16_t yOff);
const char* drawTextWordWrap(const font_t* font, paletteColor_t color, const char* text, int16_t* xOff, int16_t* yOff,
                             int16_t xMax, int16_t yMax);
uint16_t textWidth(const font_t* font, const char* text);
uint16_t textHeight(const font_t* font, const char* text, int16_t width, int16_t maxHeight);

#endif