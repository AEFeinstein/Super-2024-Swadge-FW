#pragma once
#include <stdint.h>

#include "swadge2024.h"
#include "emu_ext.h"

extern emuExtension_t modesEmuExtension;

swadgeMode_t* const* emulatorGetSwadgeModes(int* count);
const swadgeMode_t* emulatorFindSwadgeMode(const char* name);
bool emulatorSetSwadgeModeByName(const char* name);
