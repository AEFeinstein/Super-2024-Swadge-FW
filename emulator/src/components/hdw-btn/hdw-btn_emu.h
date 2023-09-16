#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "hdw-btn.h"

void emulatorInjectButton(buttonBit_t button, bool down);
void emulatorSetTouchJoystick(int32_t phi, int32_t radius, int32_t intensity);
void emulatorHandleKeys(int keycode, int bDown);
buttonBit_t emulatorGetButtonState(void);
