// Copyright 2015 <>< Charles Lohr under the ColorChord License.

#ifndef _EMBEDDED_NF_H
#define _EMBEDDED_NF_H

#include "ccconfig.h"

// Use a 32-bit DFT.  It won't work for AVRs, but for any 32-bit systems where
// they can multiply quickly, this is the bees knees.
#define USE_32DFT

#ifndef D_FREQ
    #define D_FREQ 8000
#endif

// You may make this a float. If PRECOMPUTE_FREQUENCY_TABLE is defined, then
// it will create the table at compile time, and the float will never be used
// runtime.
#define BASE_FREQ 55.0

// The higher the number the slackier your FFT will be come.
#ifndef FUZZ_IIR_BITS
    #define FUZZ_IIR_BITS 1
#endif

// Notes are the individually identifiable notes we receive from the sound.
// We track up to this many at one time.  Just because a note may appear to
// vaporize in one frame doesn't mean it is annihilated immediately.
#ifndef MAX_NOTES
    #define MAX_NOTES 12
#endif

// We take the raw signal off of the
#ifndef FILTER_BLUR_PASSES
    #define FILTER_BLUR_PASSES 2
#endif

// Determines bit shifts for where notes lie.  We represent notes with an
// uint8_t.  We have to define all of the possible locations on the note line
// in this. note_frequency = 0..((1<<SEMI_BITS_PER_BIN)*FIX_B_PER_O-1)
#ifndef SEMI_BITS_PER_BIN
    #define SEMI_BITS_PER_BIN 3
#endif

#define NOTE_RANGE ((1 << SEMI_BITS_PER_BIN) * FIX_B_PER_O)

// If there is detected note this far away from an established note, we will
// then consider this new note the same one as last time, and move the
// established note.  This is also used when combining notes.  It is this
// distance times two.
#ifndef MAX_JUMP_DISTANCE
    #define MAX_JUMP_DISTANCE 4
#endif

#ifndef MAX_COMBINE_DISTANCE
    #define MAX_COMBINE_DISTANCE 7
#endif

// These control how quickly the IIR for the note strengths respond.  AMP 1 is
// the response for the slow-response, or what we use to determine size of
// splotches, AMP 2 is the quick response, or what we use to see the visual
// strength of the notes.
#ifndef AMP_1_IIR_BITS
    #define AMP_1_IIR_BITS 4
#endif

#ifndef AMP_2_IIR_BITS
    #define AMP_2_IIR_BITS 2
#endif

// This is the amplitude, coming from folded_bins.  If the value is below this
// it is considered a non-note.
#ifndef MIN_AMP_FOR_NOTE
    #define MIN_AMP_FOR_NOTE 80
#endif

// If the strength of a note falls below this, the note will disappear, and be
// recycled back into the unused list of notes.
#ifndef MINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR
    #define MINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR 64
#endif

// This prevents compilation of any floating-point code, but it does come with
// an added restriction: Both D_FREQ and BASE_FREQ must be #defined to be
// constants.
#define PRECOMPUTE_FREQUENCY_TABLE

#include "DFT32.h"

typedef struct
{
    uint16_t folded_bins[FIX_B_PER_O]; //<! The folded fourier output.
    uint16_t fuzzed_bins[FIX_BINS];    //<! The Full DFT after IIR, Blur and Taper
    //  frequency of note; Note if it is == 255,
    // then it means it is not set. It is
    // generally a value from
    uint8_t note_peak_frequencies[MAX_NOTES];
    uint16_t note_peak_amps[MAX_NOTES];
    uint16_t note_peak_amps2[MAX_NOTES]; // (Responds quicker)
    uint8_t note_jumped_to[MAX_NOTES];   // When a note combines into another one,
    // this records where it went. I.e. if
    // your note just disappeared, check this
    // flag.
} embeddedNf_data;

void UpdateFrequencies(dft32_data* dd);                    // Not user-useful on most systems.
void HandleFrameInfo(embeddedNf_data* ed, dft32_data* dd); // Not user-useful on most systems

// Call this when starting.
void InitColorChord(embeddedNf_data* ed, dft32_data* dd);

#endif
