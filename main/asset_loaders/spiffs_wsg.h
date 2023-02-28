/*! \file spiffs_wsg.h
 *
 * \section spiffs_wsg_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section spiffs_wsg_usage Usage
 *
 * TODO doxygen
 *
 * \section spiffs_wsg_example Example
 *
 * \code{.c}
 * TODO doxygen
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