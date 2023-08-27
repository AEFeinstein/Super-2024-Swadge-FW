/*! \file ray_map_loader.h
 *
 * \section ray_map_loader_design Design Philosophy
 *
 * These functions load and free Ray Map assets which are compressed and compiled into the SPIFFS filesystem into RAM.
 * Once decompressed the data is written to a ::rayMap_t struct.
 *
 * For information on asset processing, see <a
 * href="https://github.com/AEFeinstein/Swadge-IDF-5.0/tree/main/tools/spiffs_file_preprocessor">spiffs_file_preprocessor</a>.
 *
 * \section ray_map_loader_usage Usage
 *
 * Load the map from SPIFFS to RAM using loadRayMap(). ::rayMap_t may be loaded to normal RAM, which is smaller and
 * faster, or SPI RAM, which is larger and slower.
 *
 * Free when done using freeRayMap(). If a rmh is not freed, the memory will leak.
 *
 * \section ray_map_loader_example Example
 *
 * \code{.c}
 * // Load the map data
 * rayMap_t map;
 * uint32_t posX, posY;
 * loadRayMap("demo.rmh", &map, &posX, &posY, false);
 *
 * // Free the map data
 * freeRayMap(&rmhStr);
 * \endcode
 */

#ifndef _ray_map_loader_H_
#define _ray_map_loader_H_

#include "mode_ray.h"

void loadRayMap(const char* name, ray_t* ray, bool spiRam);
void freeRayMap(rayMap_t* map);

#endif