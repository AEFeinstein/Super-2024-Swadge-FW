#pragma once

#include "emu_ext.h"
#include "hdw-btn.h"

extern emuExtension_t fuzzerEmuExtension;

void emuSetFuzzButtonsEnabled(bool enabled);
bool emuGetFuzzButtonsEnabled(void);
void emuSetFuzzTouchEnabled(bool enabled);
bool emuGetFuzzTouchEnabled(void);
void emuSetFuzzMotionEnabled(bool enabled);
bool emuGetFuzzMotionEnabled(void);
void emuSetFuzzTimeEnabled(bool enabled);
bool emuGetFuzzTimeEnabled(void);

void emuSetFuzzButtonsMask(buttonBit_t mask);
buttonBit_t emuGetFuzzButtonsMask(void);
