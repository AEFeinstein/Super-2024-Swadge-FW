/*! \file spiffs_json.h
 *
 * \section spiffs_json_design Design Philosophy
 *
 * These functions load and free JSON assets which are compressed and compiled into the SPIFFS filesystem into RAM. Once
 * decompressed and loaded into RAM, the JSON data is a string that may be parsed for whatever purpose.
 *
 * For information on asset processing, see <a
 * href="https://github.com/AEFeinstein/Swadge-IDF-5.0/tree/main/tools/spiffs_file_preprocessor">spiffs_file_preprocessor</a>.
 *
 * \section spiffs_json_usage Usage
 *
 * Load jsons from SPIFFS to RAM using loadJson(). JSON strings may be loaded to normal RAM, which is smaller and
 * faster, or SPI RAM, which is larger and slower.
 *
 * Free when done using freeJson(). If a json is not freed, the memory will leak.
 *
 * \section spiffs_json_example Example
 *
 * \code{.c}
 * char* jsonStr = loadJson("level_data.json", true);
 * // Free the json
 * freeJson(&jsonStr);
 * \endcode
 */

#ifndef _SPIFFS_JSON_H_
#define _SPIFFS_JSON_H_

char* loadJson(const char* name, bool spiRam);
void freeJson(char* jsonStr);

#endif