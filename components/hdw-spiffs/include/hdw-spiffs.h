/*! \file hdw-spiffs.h
 *
 * \section spiffs_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section spiffs_usage Usage
 *
 * TODO doxygen
 *
 * \section spiffs_example Example
 *
 * \code{.c}
 * TODO doxygen
 * \endcode
 */

#ifndef _HDW_SPIFFS_H_
#define _HDW_SPIFFS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool initSpiffs(void);
bool deinitSpiffs(void);
uint8_t* spiffsReadFile(const char* fname, size_t* outsize, bool readToSpiRam);

#endif
