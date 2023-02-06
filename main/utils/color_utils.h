#ifndef _COLOR_UTILS_H_
#define _COLOR_UTILS_H_

#include <stdint.h>
#include <stdbool.h>

#include "palette.h"
#include "hdw-led.h"

uint32_t EHSVtoHEXhelper(uint8_t hue, uint8_t sat, uint8_t val, bool applyGamma);
led_t SafeEHSVtoHEXhelper(uint8_t hue, uint8_t sat, uint8_t val, bool applyGamma);
paletteColor_t paletteHsvToHex(uint8_t hue, uint8_t sat, uint8_t val);
paletteColor_t RGBtoPalette(uint32_t rgb);
uint32_t paletteToRGB(paletteColor_t pal);

#endif
