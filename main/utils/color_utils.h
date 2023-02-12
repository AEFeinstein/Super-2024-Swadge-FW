/*! \file color_utils.h
 *
 * \section color_utils_design Design Philosophy
 *
 * These are a collection of functions to convert from red, green, blue (RGB) to <a
 * href="https://en.wikipedia.org/wiki/HSL_and_HSV">hue, saturation, value (HSV)</a> color spaces.
 *
 * Functions can also convert between the paletteColor_t enum and the RGB color space.
 *
 * \section color_utils_usage Usage
 *
 * Call the functions as necessary.
 *
 * \section color_utils_example Example
 *
 * \code{.c}
 * // HSV to RGB, three different output formats
 * uint8_t hue = 0xFF;
 * uint8_t sat = 0xFF;
 * uint8_t val = 0xFF;
 *
 * uint32_t rgb        = EHSVtoHEXhelper(hue, sat, val, true);
 * led_t ledVal        = LedEHSVtoHEXhelper(hue, sat, val, true);
 * paletteColor_t pCol = paletteHsvToHex(hue, sat, val);
 *
 * // Palette to RGB and back
 * paletteColor_t pCol = RGBtoPalette(0x123456);
 * uint32_t rgbCol     = paletteToRGB(c345);
 * \endcode
 */

#ifndef _COLOR_UTILS_H_
#define _COLOR_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

#include "palette.h"
#include "hdw-led.h"

uint32_t EHSVtoHEXhelper(uint8_t hue, uint8_t sat, uint8_t val, bool applyGamma);
led_t LedEHSVtoHEXhelper(uint8_t hue, uint8_t sat, uint8_t val, bool applyGamma);
paletteColor_t paletteHsvToHex(uint8_t hue, uint8_t sat, uint8_t val);
paletteColor_t RGBtoPalette(uint32_t rgb);
uint32_t paletteToRGB(paletteColor_t pal);

#endif
