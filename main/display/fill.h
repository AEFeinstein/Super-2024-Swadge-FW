/*! \file fill.h
 *
 * \section fill_design Design Philosophy
 *
 * Fill algorithms are used to fill bounded spaces on the display with a given color.
 * Fill algorithms can be computationally expensive, so a variety are provided to best suit your need.
 *
 * \section fill_usage Usage
 *
 * fillDisplayArea() is used to fill a rectangular area. It does not care about what is on the display prior.
 *
 * shadeDisplayArea() is used to shade a rectangular area using
 *
 * oddEvenFill() is an efficient way to fill areas using the <a
 * href="https://en.wikipedia.org/wiki/Even%E2%80%93odd_rule">Even–odd rule</a>. It may not work in all cases, but if it
 * does work, it is preferrable to use.
 *
 * floodFill() is a less efficient way to fill areas using the <a href="https://en.wikipedia.org/wiki/Flood_fill">Flood
 * fill</a> recursive algorithm. It produces better results than oddEvenFill(), but is considerably slower and uses far
 * more stack memory.
 *
 * \section fill_example Example
 *
 * \code{.c}
 * // Fill the display area with a dark cyan
 * fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c123);
 *
 * // Shade different areas with different intensities, medium brown
 * for (int i = 0; i < 5; i++)
 * {
 *     shadeDisplayArea(0, (i * TFT_HEIGHT) / 5, TFT_WIDTH / 2, ((i + 1) * TFT_HEIGHT) / 5, i, c321);
 * }
 *
 * // Draw a red circle
 * drawCircle(200, 50, 20, c500, 0, 0, 1, 1);
 * // Flood fill the circle with blue
 * floodFill(200, 50, c005, 200 - 20, 50 - 20, 200 + 50, 50 + 20);
 *
 * // Draw a green rectangle
 * drawRect(200, 150, 250, 220, c050, 0, 0, 1, 1);
 * // Odd-even fill the rectangle with blue
 * oddEvenFill(190, 140, 260, 230, c050, c005);
 * \endcode
 */

#ifndef _FILL_H_
#define _FILL_H_

#include <stdint.h>
#include <stdbool.h>

#include "palette.h"

void fillDisplayArea(int16_t x1, int16_t y1, int16_t x2, int16_t y2, paletteColor_t c);
void shadeDisplayArea(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t shadeLevel, paletteColor_t color);
void oddEvenFill(int x0, int y0, int x1, int y1, paletteColor_t boundaryColor, paletteColor_t fillColor);
void floodFill(uint16_t x, uint16_t y, paletteColor_t col, uint16_t xMin, uint16_t yMin, uint16_t xMax, uint16_t yMax);
void fillCircleSector(uint16_t x, uint16_t y, uint16_t innerR, uint16_t outerR, uint16_t startAngle, uint16_t endAngle, paletteColor_t col);

#endif