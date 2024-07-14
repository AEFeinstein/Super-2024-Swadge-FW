/*! \file cnfs.h
 *
 * \section cnfs_design Design Philosophy
 *
 * Ths Swadge loads an assets "image" as `cnfs_files` and `cnfs_data`.
 *
 * During the build, assets in the \c /assets/ folder, such as PNG images and MIDI songs, are processed into
 * Swadge-friendly formats and written to the \c /spiffs_image/ folder. The contents of the \c /spiffs_image/ are then
 * packaged into a matching `_files` and `_data` image which is stored as a C file and loaded alongside cnfs.c.
 *
 * \section cnfs_usage Usage
 *
 * You don't need to call cnfsInit() or cnfsDeinit(). The system does that the appropriate time.
 *
 * cnfsReadFile() may be used to read a file straight from cnfs, but this probably should not be done directly.
 *
 * Each asset type has it's own file loader which handles things like decompression if the asset type is compressed,
 * and writing values from the read file into a convenient struct. The loader functions are:
 *  - loadFont() & freeFont() - Load font assets from CNFS to draw text to the display
 *  - loadWsg() & freeWsg() - Load image assets from CNFS to draw images to the display
 *  - loadJson() & freeJson() - Load JSON assets from CNFS to configure games
 *  - loadTxt() & freeTxt() - Load text assets from CNFS to use in a Swadge mode
 *
 * Assets may be loaded to either SPI RAM or normal RAM.
 * There is more SPI RAM available, but it is slower to access than normal RAM.
 * Swadge modes should use normal RAM if they can, and use SPI RAM if the mode is asset-heavy.
 *
 * \section cnfs_example Example
 *
 * \code{.c}
 * // Declare and load a font
 * font_t ibm;
 * loadFont("ibm_vga8.font", &ibm, false);
 * // Draw some white text
 * drawText(&ibm, c555, "Hello World", 0, 0);
 * // Free the font
 * freeFont(&ibm);
 *
 * // Declare and load an image
 * wsg_t king_donut;
 * loadWsg("kid0.wsg", &king_donut, true);
 * // Draw the image to the display
 * drawWsg(&king_donut, 100, 100, false, false, 0);
 * // Free the image
 * freeWsg(&king_donut);
 * \endcode
 */

#ifndef _HDW_CNFS_H_
#define _HDW_CNFS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool initCnfs(void);
bool deinitCnfs(void);
uint8_t* cnfsReadFile(const char* fname, size_t* outsize, bool readToSpiRam);

#endif