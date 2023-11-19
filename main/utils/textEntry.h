#ifndef _TEXT_ENTRY_H
#define _TEXT_ENTRY_H

#include <stdint.h>
#include <stdbool.h>
#include "display/font.h"

void textEntryStart(font_t* usefont, int max_len, char* buffer);
bool textEntryDraw(void);
void textEntryEnd(void);
bool textEntryInput(uint8_t down, uint8_t button);

#endif
