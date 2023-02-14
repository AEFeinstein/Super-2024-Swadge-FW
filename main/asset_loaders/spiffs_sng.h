#ifndef _SPIFFS_SNG_H_
#define _SPIFFS_SNG_H_

#include <stdint.h>
#include <stdbool.h>

#include "hdw-bzr.h"

bool loadSng(char* name, song_t* sng, bool spiRam);
void freeSng(song_t* sng);

#endif