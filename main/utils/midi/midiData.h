#pragma once

#include "midiPlayer.h"

// Instruments
extern const midiTimbre_t acousticGrandPianoTimbre;
extern const midiTimbre_t* gmTimbres[];
extern const size_t gmTimbreCount;

extern const midiTimbre_t magfestWaveTimbre;
extern const midiTimbre_t* const magfestTimbres[];
extern const size_t magfestTimbreCount;

extern const midiTimbre_t* const mmxTimbres[];
extern const uint8_t mmxTimbreMap[];
extern const size_t mmxTimbreCount;

// extern const midiTimbre_t gmInstruments[128];

// Drumkits
extern const midiTimbre_t defaultDrumkitTimbre;
extern const midiTimbre_t donutDrumkitTimbre;
extern const midiTimbre_t mmxDrumkitTimbre;