#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "hdw-btn.h"

void emulatorInjectButton(buttonBit_t button, bool down);
void emulatorSetTouchAngleRadius(int32_t angle, int32_t radius, int32_t intensity);
void emulatorSetTouchCentroid(int32_t centerVal, int32_t intensityVal);
int32_t emulatorMapTouchCentroid(buttonBit_t buttonState);
void emulatorHandleKeys(int keycode, int bDown);
buttonBit_t emulatorGetButtonState(void);
