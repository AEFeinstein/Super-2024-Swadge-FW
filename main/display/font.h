/*! \file font.h
 *
 * \section font_design Design Philosophy
 *
 * Fonts are used to draw text to the display.
 * Each font is comprised of the ASCII characters from \c ' ' to \c '~'.
 *
 * All characters in the font have the same height.
 * Each character in the font may have it's own width.
 *
 * Each character is represented by a bit-packed bitmap where each bit is one pixel.
 * Characters may be drawn in any color.
 *
 * Fonts can be loaded from the filesystem with helper functions in fs_font.h.
 * Once loaded from the filesystem they can be used to draw text to the display.
 *
 * \section font_usage Usage
 *
 * drawText() is used to draw a line of text to the display. The line won't wrap around if it draws off the display.
 * drawTextWordWrap() can also be used to draw text to the display, and the text will wrap around if it would draw off
 * the display. This is more computationally expensive.
 *
 * drawChar() can be used to draw a single character to the display.
 *
 * textWidth() is used to measure the width of text before drawing. This is useful for centering or aligning text.
 *
 * textWordWrapHeight() is used to measure the height of a word-wrapped text block.
 * There is no function to get the height of text because it is accessible in ::font_t.height.
 *
 * \section font_example Example
 *
 * \code{.c}
 * // Declare and load a font
 * font_t ibm;
 * loadFont("ibm_vga8.font", &ibm, false);
 * // Draw some white text
 * drawText(&ibm, c555, "Hello World", 0, 0);
 * // Free the font
 * freeFont(&ibm);
 * \endcode
 */

#ifndef _FONT_H_
#define _FONT_H_

#include <stdint.h>
#include <stdbool.h>

#include "palette.h"

/**
 * @brief A character used in a font_t. Each character is a bitmap with the same height as the other characters in the
 * font.
 */
typedef struct
{
    uint8_t width;   ///< The width of this character
    uint8_t* bitmap; ///< This character's bitmap data
} font_ch_t;

/**
 * @brief A font is a collection of font_ch_t for all ASCII characters. Each character has the same height and variable
 * width.
 */
typedef struct
{
    uint8_t height;                 ///< The height of this font. All chars have the same height
    font_ch_t chars['~' - ' ' + 2]; ///< An array of characters, enough space for all printed ASCII chars, and pi
} font_t;

void drawChar(paletteColor_t color, int h, const font_ch_t* ch, int16_t xOff, int16_t yOff);
int16_t drawText(const font_t* font, paletteColor_t color, const char* text, int16_t xOff, int16_t yOff);
void drawCharBounds(paletteColor_t color, int h, const font_ch_t* ch, int16_t xOff, int16_t yOff, int16_t xMin,
                    int16_t yMin, int16_t xMax, int16_t yMax);
int16_t drawTextBounds(const font_t* font, paletteColor_t color, const char* text, int16_t xOff, int16_t yOff,
                       int16_t xMin, int16_t yMin, int16_t xMax, int16_t yMax);

const char* drawTextWordWrap(const font_t* font, paletteColor_t color, const char* text, int16_t* xOff, int16_t* yOff,
                             int16_t xMax, int16_t yMax);
const char* drawTextWordWrapFixed(const font_t* font, paletteColor_t color, const char* text, int16_t xStart,
                                  int16_t yStart, int16_t* xOff, int16_t* yOff, int16_t xMax, int16_t yMax);
uint16_t textWidth(const font_t* font, const char* text);
uint16_t textWordWrapHeight(const font_t* font, const char* text, int16_t width, int16_t maxHeight);

void makeOutlineFont(font_t* srcFont, font_t* dstFont, bool spiRam);
int16_t drawTextMarquee(const font_t* font, paletteColor_t color, const char* text, int16_t xOff, int16_t yOff,
                        int16_t xMax, int64_t* timer);
bool drawTextEllipsize(const font_t* font, paletteColor_t color, const char* text, int16_t xOff, int16_t yOff,
                       int16_t maxW);

int16_t drawTextMulticolored(const font_t* font, const char* text, int16_t xOff, int16_t yOff, paletteColor_t* colors,
                             uint32_t colorCount, uint32_t segmentCount);

#endif