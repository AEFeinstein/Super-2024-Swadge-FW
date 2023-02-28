/*! \file cndraw.h
 * \authors adam, CNLohr
 * \date Sep 26, 2020
 *
 * \section cndraw_design Design Philosophy
 *
 * TODO doxygen
 *
 * \section cndraw_usage Usage
 *
 * TODO doxygen
 *
 * \section cndraw_example Example
 *
 * \code{.c}
 * TODO doxygen
 * \endcode
 */

#ifndef CNDRAW_H_
#define CNDRAW_H_

#include <stdint.h>

#include "palette.h"


void outlineTriangle(int16_t v0x, int16_t v0y, int16_t v1x, int16_t v1y, int16_t v2x, int16_t v2y,
                     paletteColor_t colorA, paletteColor_t colorB);

void speedyLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, paletteColor_t color);

#endif
