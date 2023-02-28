/*! \file spiffs_font.h
 *
 * \section spiffs_font_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section spiffs_font_usage Usage
 *
 * TODO doxygen
 *
 * \section spiffs_font_example Example
 *
 * \code{.c}
 * TODO doxygen
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