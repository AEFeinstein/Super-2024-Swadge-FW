#ifndef _FILL_H_
#define _FILL_H_

#include <stdint.h>
#include "palette.h"

void oddEvenFill(int x0, int y0, int x1, int y1, paletteColor_t boundaryColor, paletteColor_t fillColor);
void floodFill(uint16_t x, uint16_t y, paletteColor_t col, uint16_t xMin, uint16_t yMin, uint16_t xMax, uint16_t yMax);

#endif