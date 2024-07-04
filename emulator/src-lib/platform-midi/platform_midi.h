#ifndef _PLATFORM_MIDI_H_
#define _PLATFORM_MIDI_H_

#define PLATFORM_MIDI_SUPPORTED 1

#if defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__)
#define PLATFORM_MIDI_ALSA 1
#elif defined(__APPLE__)
#define PLATFORM_MIDI_COREMIDI 1
#undef PLATFORM_MIDI_SUPPORTED
#endif


#if PLATFORM_MIDI_ALSA
#include "platform_midi_alsa.h"
#endif

#if PLATFORM_MIDI_COREMIDI
#include "platform_midi_coremidi.h"
#endif


#endif
