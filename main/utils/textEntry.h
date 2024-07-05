#ifndef _TEXT_ENTRY_H
#define _TEXT_ENTRY_H

#include <stdint.h>
#include <stdbool.h>
#include "display/font.h"
#include "swadge2024.h"

void textEntryStart(font_t* usefont, int max_len, char* buffer);
void textEntryStartPretty(font_t* usefont, int max_len, char* buffer, wsg_t BG, uint8_t tbColor, uint8_t txtColor);
bool textEntryDraw(int64_t elapsedUs);
bool textEntryInput(uint8_t down, uint8_t button);
static void _drawKeyboard(void);
static void _drawCaps(int16_t x, int16_t y, uint8_t color);
static void _drawShift(int16_t x, int16_t y, uint8_t color);
static void _drawBackspace(int16_t x, int16_t y, uint8_t color);
static void _drawSpacebar(int16_t x, int16_t y, uint8_t color);
static void _drawTab(int16_t x, int16_t y, uint8_t color);
static void _drawEnter(int16_t x, int16_t y, uint8_t color);

#endif
