/*! \file spiffs_sng.h
 *
 * \section spiffs_song_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section spiffs_song_usage Usage
 *
 * TODO doxygen
 *
 * \section spiffs_song_example Example
 *
 * \code{.c}
 * TODO doxygen
 * \endcode
 */

#ifndef _SPIFFS_SNG_H_
#define _SPIFFS_SNG_H_

#include <stdint.h>
#include <stdbool.h>

#include "hdw-bzr.h"

bool loadSng(char* name, song_t* sng, bool spiRam);
void freeSng(song_t* sng);

#endif