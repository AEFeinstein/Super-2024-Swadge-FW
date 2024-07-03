// Copyright 2015 <>< Charles Lohr under the ColorChord License.

#ifndef _DFT32_H
#define _DFT32_H

#include <stdint.h>
#include "ccconfig.h"

// A 32-bit version of the DFT used for ColorChord.
// This header makes it convenient to use for an embedded system.
// The 32-bit DFT avoids some bit shifts, however it uses slightly
// more RAM and it uses a lot of 32-bit arithmetic.
//
// This is basically a clone of "ProgressiveIntegerSkippy" and changes
// made here should be back-ported there as well.

// You can # define these to be other things elsewhere.

// Will used simple approximation of norm rather than
//   sum squares and approx sqrt
#ifndef APPROX_NORM
    #define APPROX_NORM 1
#endif

#ifndef OCTAVES
    #define OCTAVES 5
#endif

#ifndef FIX_B_PER_O
    #define FIX_B_PER_O 24
#endif

// Don't configure this.
#define FIX_BINS  (FIX_B_PER_O * OCTAVES)
#define BIN_CYCLE (1 << OCTAVES)

// You may increase this past 5 but if you do, the amplitude of your incoming
// signal must decrease.  Increasing this value makes responses slower.  Lower
// values are more responsive.
#ifndef DFT_IIR
    #define DFT_IIR 6
#endif

// Everything the integer one buys, except it only calculates 2 octaves worth of
// notes per audio frame.
// This is sort of working, but still have some quality issues.
// It would theoretically be fast enough to work on an AVR.
// NOTE: This is the only DFT available to the embedded port of ColorChord
#ifndef CC_EMBEDDED
void DoDFTProgressive32(dft32_data* dd, float* outBins, float* frequencies, int bins, const float* dataBuffer,
                        int place_in_data_buffer, int size_of_data_buffer, float q, float speedup);
#endif

typedef struct
{
    // Whenever you need to read the bins, you can do it from here.
    // These outputs are limited to 0..~2047, this makes it possible
    // for you to process with uint16_t's more easily.
    // This is updated every time the DFT hits the octave count, or 1/32 updates.
    uint16_t embeddedBins32[FIX_BINS];

    // NOTES to self:
    //
    //  Let's say we want to try this on an AVR.
    //   24 bins, 5 octaves = 120 bins.
    //  20 MHz clock / 4.8k sps = 4096 IPS = 34 clocks per bin = :(
    //   We can do two at the same time, this frees us up some
    uint8_t sDoneFirstRun;

    // (advances,places) full revolution is 256. 8bits integer part 8bit fractional
    uint16_t sDatSpace32A[FIX_BINS * 2];
    // (isses,icses)
    int32_t sDatSpace32B[FIX_BINS * 2];

    // This is updated every time the DFT hits the octave count, or 1 out of
    // (1<<OCTAVES) times which is (1<<(OCTAVES-1)) samples
    // (isses,icses)
    int32_t sDatSpace32BOut[FIX_BINS * 2];

    // Sdo_this_octave is a scheduling state for the running SIN/COS states for
    // each bin.  We have to execute the highest octave every time, however, we can
    // get away with updating the next octave down every-other-time, then the next
    // one down yet, every-other-time from that one.  That way, no matter how many
    // octaves we have, we only need to update FIX_B_PER_O*2 DFT bins.
    uint8_t Sdo_this_octave[BIN_CYCLE];

    int32_t sAccum_octave_bins[OCTAVES];
    uint8_t sWhichOctavePlace;

    uint16_t embeddedBins[FIX_BINS];
} dft32_data;

// It's actually split into a few functions, which you can call on your own:
int SetupDFTProgressive32(dft32_data* dd); // Call at start. Returns nonzero if error.
void UpdateBins32(dft32_data* dd, const uint16_t* frequencies);

// Call this to push on new frames of sound.
// Though it accepts an int16, it actually only takes -4095 to +4095. (13-bit)
// Any more and you will exceed the accumulators and it will cause an overflow.
void PushSample32(dft32_data* dd, int16_t dat);

#ifndef CC_EMBEDDED
// ColorChord regular uses this to pass in floats.
void UpdateBinsForDFT32(dft32_data* dd, const float* frequencies); // Update the frequencies
#endif

// This takes the current sin/cos state of ColorChord and output to
// embeddedBins32.
void UpdateOutputBins32(dft32_data* dd);

#endif
