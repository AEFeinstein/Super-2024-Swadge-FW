/*! \file ext_midi.h
 *
 * \section ext_midi Extended Emulator MIDI Support
 */

#pragma once

#include "emu_ext.h"

extern emuExtension_t midiEmuExtension;

bool emuExportMidiToWav(const char* filename);
