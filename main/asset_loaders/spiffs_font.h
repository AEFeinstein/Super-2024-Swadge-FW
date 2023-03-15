/*! \file spiffs_font.h
 *
 * \section spiffs_font_design Design Philosophy
 *
 * These functions load and free font assets which are compiled into the SPIFFS filesystem into RAM. Once loaded into
 * RAM, fonts may be used to draw text to the screen.
 *
 * For more information about using fonts, see font.h.
 *
 * For information on asset processing, see <a
 * href="https://github.com/AEFeinstein/Swadge-IDF-5.0/tree/main/tools/spiffs_file_preprocessor">spiffs_file_preprocessor</a>.
 *
 * \section spiffs_font_usage Usage
 *
 * Load fonts from SPIFFS to RAM using loadFont(). Fonts may be loaded to normal RAM, which is smaller and faster, or
 * SPI RAM, which is larger and slower.
 *
 * Free when done using freeFont(). If a font is not freed, the memory will leak.
 *
 * \section spiffs_font_example Example
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

#ifndef _SPIFFS_FONT_H_
#define _SPIFFS_FONT_H_

#include <stdint.h>
#include <stdbool.h>

#include "font.h"

bool loadFont(const char* name, font_t* font, bool spiRam);
void freeFont(font_t* font);

#endif