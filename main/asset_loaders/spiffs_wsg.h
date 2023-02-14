#ifndef _SPIFFS_WSG_H_
#define _SPIFFS_WSG_H_

#include <stdint.h>
#include <stdbool.h>

#include "palette.h"

#include "wsg.h"

bool loadWsg(char* name, wsg_t* wsg);
bool loadWsgSpiRam(char* name, wsg_t* wsg, bool spiRam);
void freeWsg(wsg_t* wsg);

#endif