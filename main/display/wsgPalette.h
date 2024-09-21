/**
 * @file wsgPalette.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Provides palette swap functionality for Swadge
 * @version 1.0.0
 * @date 2024-09-20
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include <palette.h>
#include <stdint.h>
#include "wsg.h"

//==============================================================================
// Struct
//==============================================================================

typedef struct
{
    paletteColor_t newColors[217];
} wsgPalette_t;

//==============================================================================
// Functions
//==============================================================================

void drawWsgPalette(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette, bool flipLR, bool flipUD, int16_t rotateDeg);
void wsgPaletteReset(wsgPalette_t* palette);
void wsgPaletteSet(wsgPalette_t* palette, paletteColor_t replaced, paletteColor_t newColor);