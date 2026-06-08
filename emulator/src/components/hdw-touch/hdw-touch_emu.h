#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "hdw-touch.h"

void emulatorSetTouchJoystick(int32_t phi, int32_t radius, int32_t intensity);
void emulatorSetTouchLinear(uint32_t arrIdx, int32_t position, int32_t intensity);
