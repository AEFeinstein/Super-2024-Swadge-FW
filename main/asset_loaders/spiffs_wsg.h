/*! \file spiffs_wsg.h
 *
 * \section spiffs_wsg_design Design Philosophy
 *
 * These functions load and free WSG assets which are images that are compressed and compiled into the SPIFFS filesystem
 * into RAM. Once decompressed loaded into RAM, WSGs may be drawn to the display.
 *
 * For more information about using WSGs, see wsg.h.
 *
 * For information on asset processing, see <a
 * href="https://github.com/AEFeinstein/Swadge-IDF-5.0/tree/main/tools/spiffs_file_preprocessor">spiffs_file_preprocessor</a>.
 *
 * \section spiffs_wsg_usage Usage
 *
 * Load WSGs from SPIFFS to RAM using loadWsg(). WSGs may be loaded to normal RAM, which is smaller and faster, or
 * SPI RAM, which is larger and slower.
 *
 * Free when done using freeWsg(). If a wsg is not freed, the memory will leak.
 *
 * \section spiffs_wsg_example Example
 *
 * \code{.c}
 * // Declare and load a WSG
 * wsg_t king_donut;
 * loadWsg("kid0.wsg", &king_donut, true);
 * // Draw the WSGto the display
 * drawWsg(&king_donut, 100, 10, false, false, 0);
 * // Free the WSG
 * freeWsg(&king_donut);
 * \endcode
 */

#ifndef _SPIFFS_WSG_H_
#define _SPIFFS_WSG_H_

#include <stdint.h>
#include <stdbool.h>

#include "palette.h"

#include "wsg.h"

bool loadWsg(char* name, wsg_t* wsg, bool spiRam);
void freeWsg(wsg_t* wsg);

#endif