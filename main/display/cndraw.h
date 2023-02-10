/*
 * cndraw.h
 *
 *  Created on: Sep 26, 2020-2022
 *      Author: adam, CNLohr
 */

#ifndef CNDRAW_H_
#define CNDRAW_H_

#include <stdint.h>

#include "palette.h"

#ifdef EMU
extern uint32_t cndrawPerfcounter;
#endif

void shadeDisplayArea(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t shadeLevel, paletteColor_t color);

void outlineTriangle(int16_t v0x, int16_t v0y, int16_t v1x, int16_t v1y, int16_t v2x, int16_t v2y,
                     paletteColor_t colorA, paletteColor_t colorB);

void speedyLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, paletteColor_t color);

#endif
