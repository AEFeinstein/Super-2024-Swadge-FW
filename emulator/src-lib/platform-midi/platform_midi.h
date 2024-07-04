#ifndef _PLATFORM_MIDI_H_
#define _PLATFORM_MIDI_H_

#define PLATFORM_MIDI_SUPPORTED 1

#if defined(__linux) || defined(__linux__) || defined(linux) || defined(__LINUX__)
#define PLATFORM_MIDI_ALSA 1
#else
#undef PLATFORM_MIDI_SUPPORTED
#endif


#if PLATFORM_MIDI_ALSA
#include "platform_midi_alsa.h"
#endif

#endif
