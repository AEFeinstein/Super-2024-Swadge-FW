/*! \file fs_font.h
 *
 * \section fs_font_design Design Philosophy
 *
 * These functions load and free font assets which are compiled into the filesystem into RAM. Once loaded into
 * RAM, fonts may be used to draw text to the screen.
 *
 * For more information about using fonts, see font.h.
 *
 * For information on asset processing, see <a
 * href="https://github.com/AEFeinstein/Super-2024-Swadge-FW/tree/main/tools/assets_preprocessor">assets_preprocessor</a>.
 *
 * \section fs_font_usage Usage
 *
 * Load fonts from the filesystem to RAM using loadFont(). Fonts may be loaded to normal RAM, which is smaller and
 * faster, or SPI RAM, which is larger and slower.
 *
 * Free when done using freeFont(). If a font is not freed, the memory will leak.
 *
 * \section fs_font_example Example
 *
 * \code{.c}
 * // Declare and load a font
 * font_t ibm;
 * loadFont(IBM_VGA_8_FONT, &ibm, false);
 * // Draw some white text
 * drawText(&ibm, c555, "Hello World", 0, 0);
 * // Free the font
 * freeFont(&ibm);
 * \endcode
 */

#ifndef _FS_FONT_H_
#define _FS_FONT_H_

#include <stdint.h>
#include <stdbool.h>

#include "cnfs_image.h"
#include "font.h"

bool loadFont(cnfsFileIdx_t fIdx, font_t* font, bool spiRam);
void freeFont(font_t* font);

#endif