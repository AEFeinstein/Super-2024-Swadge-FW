#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <vector2d.h>
#include "gs_typedef.h"

//==============================================================================
// Prototypes
//==============================================================================
int gs_lerp(int a, int b, uint16_t amount);
int16_t gs_logRemap(int16_t x);
int gs_randomInt(int lowerBound, int upperBound);
bool gs_checkBit(uint32_t var, uint8_t pos);