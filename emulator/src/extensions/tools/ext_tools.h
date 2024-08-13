/*! \file ext_tools.h
 *
 * \section ext_tools Helpful Tools in the Emulator
 *
 * \subsection ext_tools_screenshot Screenshots
 * To take a screenshot, press the `F12` key at any time. A screenshot will
 * be saved to the current directory with the name in the format 'screenshot-1712953237703.png`
 */

#pragma once

#include "emu_ext.h"

#include <stddef.h>

extern emuExtension_t toolsEmuExtension;

bool takeScreenshot(const char* name);
void startScreenRecording(const char* name);
void stopScreenRecording(void);
bool isScreenRecording(void);