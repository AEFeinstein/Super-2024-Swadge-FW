/*! \file spiffs_rmh.h
 *
 * \section spiffs_rmh_design Design Philosophy
 *
 * These functions load and free Ray Map assets which are compressed and compiled into the SPIFFS filesystem into RAM.
 * Once decompressed the data is written to a ::rayMap_t struct.
 *
 * For information on asset processing, see <a
 * href="https://github.com/AEFeinstein/Swadge-IDF-5.0/tree/main/tools/spiffs_file_preprocessor">spiffs_file_preprocessor</a>.
 *
 * \section spiffs_rmh_usage Usage
 *
 * Load the map from SPIFFS to RAM using loadRmh(). ::rayMap_t may be loaded to normal RAM, which is smaller and
 * faster, or SPI RAM, which is larger and slower.
 *
 * Free when done using freeRmh(). If a rmh is not freed, the memory will leak.
 *
 * \section spiffs_rmh_example Example
 *
 * \code{.c}
 * // Load the map data
 * rayMap_t map;
 * uint32_t posX, posY;
 * loadRmh("demo.rmh", &map, &posX, &posY, false);
 *
 * // Free the map data
 * freeRmh(&rmhStr);
 * \endcode
 */

#ifndef _SPIFFS_RMH_H_
#define _SPIFFS_RMH_H_

#include "mode_ray.h"

void loadRmh(const char* name, rayMap_t* map, int32_t* startX, int32_t* startY, bool spiRam);
void freeRmh(rayMap_t* map);

#endif