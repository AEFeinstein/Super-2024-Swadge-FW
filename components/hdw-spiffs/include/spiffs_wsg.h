#ifndef _SPIFFS_WSG_H_
#define _SPIFFS_WSG_H_

#include <stdint.h>
#include <stdbool.h>

#include "palette.h"

typedef struct
{
    paletteColor_t* px;
    uint16_t w;
    uint16_t h;
} wsg_t;

bool loadWsg(char* name, wsg_t* wsg);
bool loadWsgSpiRam(char* name, wsg_t* wsg, bool spiRam);
void freeWsg(wsg_t* wsg);

#endif