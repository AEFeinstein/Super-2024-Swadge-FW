#pragma once

#include <palette.h>
#include <stdint.h>
#include "wsg.h"

typedef struct
{
    paletteColor_t replacedColors[217];
    paletteColor_t newColors[217];
} wsgPalette_t;

// Palette
void drawWsgPalette(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette, bool flipLR, bool flipUD, int16_t rotateDeg);
void wsgPaletteReset(wsgPalette_t* palette);
void wsgPaletteGenerate(wsgPalette_t* palette);
void wsgPaletteSet(wsgPalette_t* palette, paletteColor_t replaced, paletteColor_t newColor);