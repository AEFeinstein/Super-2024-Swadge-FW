#pragma once

#include "emu_ext.h"

#include <hdw-btn.h>

extern emuExtension_t gamepadEmuExtension;

bool emuGamepadConnected(void);

void emuSetGamepadButtonMapping(uint8_t buttonIdx, buttonBit_t button);
void emuSetTouchpadAxisMapping(int xAxis, int yAxis);
void emuSetAccelAxisMapping(int xAxis, int yAxis, int zAxis);
void emuSetDpadAxisMapping(int xAxis, int yAxis);
