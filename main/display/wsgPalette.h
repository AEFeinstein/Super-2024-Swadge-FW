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

/*! \file wsgPalette.h
 *
 * \section wsgPalette_design Design Philosophy
 *
 * Provides functionality to WSGs to use a palette a la the NES. See wsg.h for how these are implemented.
 *
 * Clones all the current options for drawing WSGs, but all of them require a new parameter, 'palette' which is a array
 of paletteColor_t objects. The functions largely just intercepts the color given by the WSG and converts it based on
 the newColor map.
 *
 * \section wsgPalette_usage Usage
 *
 * There are three setup functions:
 * - wsgPaletteReset(): Resets the provided palette to draw the default colors
 * - wsgPaletteSet(): Provided the palette, a color to overwrite and a new color to use, sets the color
 * - wsgPaletteSetGroup(): Does the same as above, but using lists to make generation easier
 *
 * If wsgPaletteReset() isn't called for the palette being used, all colors not specifically assigned will be black.
 *
 * There are four drawing functions provided with the palette
 * - drawWsgPalette(): Draws the WSG with the appropriate palette
 * - drawWsgPaletteSimple(): Draws the WSG with palette, but can't be rotated or flipped.
 * - drawWsgPaletteSimpleScaled(): Draws the WSG with palette at a larger size set by the provided scale (integer
 values, 2x, 3x, 4x...).
 * - drawWsgPaletteSimpleHalf(): Draws the WSG at half scale with the included palette.
 *
 * \section wsgPalette_example Example
 *
 * \code{.c}
 * // In modeData_t
 * {
 *     wsgPalette_t pal;
 * }

 * // In modeEnter
 * {
 *     // Palette setup
 *     wsgPaletteReset(&pal);
 *     wsgPaletteSet(&pal, c000, c555);
 * }
 *
 * // Where the WSG is drawn
 * {
 *     drawWsgPalette(&wsg, x, y, &pal, vertFlip, HorFlip, rotation);
 * }
 * \endcode
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
    paletteColor_t newColors[217]; ///< Color map
} wsgPalette_t;

//==============================================================================
// Functions
//==============================================================================

void drawWsgPalette(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette, bool flipLR, bool flipUD,
                    int16_t rotateDeg);
void drawWsgPaletteSimple(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette);
void drawWsgPaletteSimpleScaled(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette, int16_t xScale,
                                int16_t yScale);
void drawWsgPaletteSimpleHalf(const wsg_t* wsg, int16_t xOff, int16_t yOff, wsgPalette_t* palette);
void wsgPaletteReset(wsgPalette_t* palette);
void wsgPaletteSet(wsgPalette_t* palette, paletteColor_t replaced, paletteColor_t newColor);
void wsgPaletteSetGroup(wsgPalette_t* palette, paletteColor_t* replacedColors, paletteColor_t* newColors,
                        uint8_t arrSize);