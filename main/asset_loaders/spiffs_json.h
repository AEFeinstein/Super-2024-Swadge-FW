/*! \file spiffs_json.h
 *
 * \section spiffs_json_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section spiffs_json_usage Usage
 *
 * TODO doxygen
 *
 * \section spiffs_json_example Example
 *
 * \code{.c}
 * TODO doxygen
 * \endcode
 */

#ifndef _SPIFFS_JSON_H_
#define _SPIFFS_JSON_H_

char* loadJson(const char* name, bool spiRam);
void freeJson(char* jsonStr);

#endif