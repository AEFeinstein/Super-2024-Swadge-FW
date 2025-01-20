#pragma once

#include "emu_ext.h"

extern const emuExtension_t screensaverEmuExtension;

void emuScreensaverActivate(void);
void emuScreensaverDeactivate(void);
void emuScreensaverWake(void);
void emuScreensaverNext(void);
