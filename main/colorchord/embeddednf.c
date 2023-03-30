// Copyright 2015 <>< Charles Lohr under the ColorChord License.

//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include "embeddedNf.h"
#include "DFT32.h"

//==============================================================================
// Constant data
//==============================================================================

#ifndef PRECOMPUTE_FREQUENCY_TABLE
static const float bf_table[24] = {1.000000, 1.029302, 1.059463, 1.090508, 1.122462, 1.155353, 1.189207, 1.224054,
                                   1.259921, 1.296840, 1.334840, 1.373954, 1.414214, 1.455653, 1.498307, 1.542211,
                                   1.587401, 1.633915, 1.681793, 1.731073, 1.781797, 1.834008, 1.887749, 1.943064};

/* The above table was generated using the following code:

#include <stdio.h>
#include <math.h>

int main()
{
    int i;
    #define FIX_B_PER_O 24
    printf( "const float bf_table[%d] = {", FIX_B_PER_O );
    for( i = 0; i < FIX_B_PER_O; i++ )
    {
        if( ( i % 6 ) == 0 )
            printf( "\n\t" );
        printf( "%f, ", pow( 2, (float)i / (float)FIX_B_PER_O ) );
    }
    printf( "};\n" );
    return 0;
}
*/

#endif

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param dd
 */
void UpdateFrequencies(dft32_data* dd)
{
#ifndef PRECOMPUTE_FREQUENCY_TABLE

    #define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2 * !!(condition)]))

    uint16_t fbins[FIX_B_PER_O];
    int i;

    BUILD_BUG_ON(sizeof(bf_table) != FIX_B_PER_O * 4);

    // Warning: This does floating point.  Avoid doing this frequently.  If you
    // absolutely cannot have floating point on your system, you may precompute
    // this and store it as a table.  It does preclude you from changing
    // BASE_FREQ in runtime.

    for (i = 0; i < FIX_B_PER_O; i++)
    {
        float frq = (bf_table[i] * BASE_FREQ);
        fbins[i]  = (65536.0) / (D_FREQ)*frq * 16 + 0.5;
    }
#else

    #define PCOMP(f) (uint16_t)((65536.0) / (D_FREQ) * (f * BASE_FREQ) * 16 + 0.5)

    static const uint16_t fbins[FIX_B_PER_O]
        = {PCOMP(1.000000), PCOMP(1.029302), PCOMP(1.059463), PCOMP(1.090508), PCOMP(1.122462), PCOMP(1.155353),
           PCOMP(1.189207), PCOMP(1.224054), PCOMP(1.259921), PCOMP(1.296840), PCOMP(1.334840), PCOMP(1.373954),
           PCOMP(1.414214), PCOMP(1.455653), PCOMP(1.498307), PCOMP(1.542211), PCOMP(1.587401), PCOMP(1.633915),
           PCOMP(1.681793), PCOMP(1.731073), PCOMP(1.781797), PCOMP(1.834008), PCOMP(1.887749), PCOMP(1.943064)};
#endif

#ifdef USE_32DFT
    UpdateBins32(dd, fbins);
#else
    UpdateBinsForProgressiveIntegerSkippyInt(fbins);
#endif
}

void InitColorChord(embeddedNf_data* ed, dft32_data* dd)
{
    int i;
    // Set up and initialize arrays.
    for (i = 0; i < MAX_NOTES; i++)
    {
        ed->note_peak_frequencies[i] = 255;
        ed->note_peak_amps[i]        = 0;
        ed->note_peak_amps2[i]       = 0;
    }

    memset(ed->folded_bins, 0, sizeof(ed->folded_bins));
    memset(ed->fuzzed_bins, 0, sizeof(ed->fuzzed_bins));

    // Step 1: Initialize the Integer DFT.
#ifdef USE_32DFT
    SetupDFTProgressive32(dd);
#else
    SetupDFTProgressiveIntegerSkippy();
#endif

    // Step 2: Set up the frequency list.  You could do this multiple times
    // if you want to change the loadout of the frequencies.
    UpdateFrequencies(dd);
}

/**
 * @brief TODO
 *
 * @param ed
 * @param dd
 */
void HandleFrameInfo(embeddedNf_data* ed, dft32_data* dd)
{
    int i, j, k;
    uint8_t hitnotes[MAX_NOTES];
    memset(hitnotes, 0, sizeof(hitnotes));

#ifdef USE_32DFT
    uint16_t* strens;
    UpdateOutputBins32(dd);
    strens = dd->embeddedBins32;
#else
    uint16_t* strens = embeddedBins;
#endif

    // Copy out the bins from the DFT to our fuzzed bins.
    for (i = 0; i < FIX_BINS; i++)
    {
        ed->fuzzed_bins[i]
            = (ed->fuzzed_bins[i] + (strens[i] >> FUZZ_IIR_BITS) - (ed->fuzzed_bins[i] >> FUZZ_IIR_BITS));
    }

    // Taper first octave
    for (i = 0; i < FIX_B_PER_O; i++)
    {
        uint32_t taperamt  = (65536 / FIX_B_PER_O) * i;
        ed->fuzzed_bins[i] = (taperamt * ed->fuzzed_bins[i]) >> 16;
    }

    // Taper last octave
    for (i = 0; i < FIX_B_PER_O; i++)
    {
        int newi              = FIX_BINS - i - 1;
        uint32_t taperamt     = (65536 / FIX_B_PER_O) * i;
        ed->fuzzed_bins[newi] = (taperamt * ed->fuzzed_bins[newi]) >> 16;
    }

    // Fold the bins from fuzzedbins into one octave.
    for (i = 0; i < FIX_B_PER_O; i++)
    {
        ed->folded_bins[i] = 0;
    }
    k = 0;
    for (j = 0; j < OCTAVES; j++)
    {
        for (i = 0; i < FIX_B_PER_O; i++)
        {
            ed->folded_bins[i] += ed->fuzzed_bins[k++];
        }
    }

    // Now, we must blur the folded bins to get a good result.
    // Sometimes you may notice every other bin being out-of
    // line, and this fixes that.  We may consider running this
    // more than once, but in my experience, once is enough.
    for (j = 0; j < FILTER_BLUR_PASSES; j++)
    {
        // Extra scoping because this is a large on-stack buffer.
        uint16_t folded_out[FIX_B_PER_O];
        uint8_t adjLeft  = FIX_B_PER_O - 1;
        uint8_t adjRight = 1;
        for (i = 0; i < FIX_B_PER_O; i++)
        {
            uint16_t lbin = ed->folded_bins[adjLeft] >> 2;
            uint16_t rbin = ed->folded_bins[adjRight] >> 2;
            uint16_t tbin = ed->folded_bins[i] >> 1;
            folded_out[i] = lbin + rbin + tbin;

            // We do this funny dance to avoid a modulus operation.  On some
            // processors, a modulus operation is slow.  This is cheap.
            adjLeft++;
            if (adjLeft == FIX_B_PER_O)
            {
                adjLeft = 0;
            }
            adjRight++;
            if (adjRight == FIX_B_PER_O)
            {
                adjRight = 0;
            }
        }

        for (i = 0; i < FIX_B_PER_O; i++)
        {
            ed->folded_bins[i] = folded_out[i];
        }
    }

    // Next, we have to find the peaks, this is what "decompose" does in our
    // normal tool.  As a warning, it expects that the values in foolded_bins
    // do NOT exceed 32767.
    {
        uint8_t adjLeft  = FIX_B_PER_O - 1;
        uint8_t adjRight = 1;
        for (i = 0; i < FIX_B_PER_O; i++)
        {
            int16_t prev     = ed->folded_bins[adjLeft];
            int16_t next     = ed->folded_bins[adjRight];
            int16_t this     = ed->folded_bins[i];
            uint8_t thisfreq = i << SEMI_BITS_PER_BIN;
            int16_t offset;
            adjLeft++;
            if (adjLeft == FIX_B_PER_O)
            {
                adjLeft = 0;
            }
            adjRight++;
            if (adjRight == FIX_B_PER_O)
            {
                adjRight = 0;
            }
            if (this < MIN_AMP_FOR_NOTE)
            {
                continue;
            }
            if (prev > this || next > this)
            {
                continue;
            }
            if (prev == this && next == this)
            {
                continue;
            }

            // i is at a peak...
            int32_t totaldiff = ((this - prev) + (this - next));
            int32_t porpdiffP = ((this - prev) << 16) / totaldiff; // close to 0 =
            // closer to this side, 32768 = in the middle, 65535 away.
            int32_t porpdiffN = ((this - next) << 16) / totaldiff;

            if (porpdiffP < porpdiffN)
            {
                // Closer to prev.
                offset = -(32768 - porpdiffP);
            }
            else
            {
                // Closer to next
                offset = (32768 - porpdiffN);
            }

            // Need to round.  That's what that extra +(15.. is in the center.
            thisfreq += (offset + (1 << (15 - SEMI_BITS_PER_BIN))) >> (16 - SEMI_BITS_PER_BIN);

            // In the event we went 'below zero' need to wrap to the top.
            if (thisfreq > 255 - (1 << SEMI_BITS_PER_BIN))
            {
                thisfreq = (1 << SEMI_BITS_PER_BIN) * FIX_B_PER_O - (256 - thisfreq);
            }

            // Okay, we have a peak, and a frequency. Now, we need to search
            // through the existing notes to see if we have any matches.
            // If we have a note that's close enough, we will try to pull it
            // closer to us and boost it.
            int8_t lowest_found_free_note = -1;
            int8_t closest_note_id        = -1;
            int16_t closest_note_distance = 32767;

            for (j = 0; j < MAX_NOTES; j++)
            {
                uint8_t nf = ed->note_peak_frequencies[j];

                if (nf == 255)
                {
                    if (lowest_found_free_note == -1)
                    {
                        lowest_found_free_note = j;
                    }
                    continue;
                }

                int16_t distance = thisfreq - nf;

                if (distance < 0)
                {
                    distance = -distance;
                }

                // Make sure that if we've wrapped around the right side of the
                // array, we can detect it and loop it back.
                if (distance > ((1 << (SEMI_BITS_PER_BIN - 1)) * FIX_B_PER_O))
                {
                    distance = ((1 << (SEMI_BITS_PER_BIN)) * FIX_B_PER_O) - distance;
                }

                // If we find a note closer to where we are than any of the
                // others, we can mark it as our desired note.
                if (distance < closest_note_distance)
                {
                    closest_note_id       = j;
                    closest_note_distance = distance;
                }
            }

            int8_t marked_note = -1;

            if (closest_note_distance <= MAX_JUMP_DISTANCE)
            {
                // We found the note we need to augment.
                ed->note_peak_frequencies[closest_note_id] = thisfreq;
                marked_note                                = closest_note_id;
            }

            // The note was not found.
            else if (lowest_found_free_note != -1)
            {
                ed->note_peak_frequencies[lowest_found_free_note] = thisfreq;
                marked_note                                       = lowest_found_free_note;
            }

            // If we found a note to attach to, we have to use the IIR to
            // increase the strength of the note, but we can't exactly snap
            // it to the new strength.
            if (marked_note != -1)
            {
                hitnotes[marked_note] = 1;

                ed->note_peak_amps[marked_note] = ed->note_peak_amps[marked_note]
                                                  - (ed->note_peak_amps[marked_note] >> AMP_1_IIR_BITS)
                                                  + (this >> (AMP_1_IIR_BITS - 3));

                ed->note_peak_amps2[marked_note] = ed->note_peak_amps2[marked_note]
                                                   - (ed->note_peak_amps2[marked_note] >> AMP_2_IIR_BITS)
                                                   + ((this << 3) >> (AMP_2_IIR_BITS));
            }
        }
    }

#if 0
    for( i = 0; i < MAX_NOTES; i++ )
    {
        if( ed->note_peak_frequencies[i] == 255 )
        {
            continue;
        }
        printf( "%d / ", ed->note_peak_amps[i] );
    }
    printf( "\n" );
#endif

    // Now we need to handle combining notes.
    for (i = 0; i < MAX_NOTES; i++)
        for (j = 0; j < i; j++)
        {
            // We'd be combining nf2 (j) into nf1 (i) if they're close enough.
            uint8_t nf1      = ed->note_peak_frequencies[i];
            uint8_t nf2      = ed->note_peak_frequencies[j];
            int16_t distance = nf1 - nf2;

            if (nf1 == 255 || nf2 == 255)
            {
                continue;
            }

            if (distance < 0)
            {
                distance = -distance;
            }

            // If it wraps around above the halfway point, then we're closer to it
            // on the other side.
            if (distance > ((1 << (SEMI_BITS_PER_BIN - 1)) * FIX_B_PER_O))
            {
                distance = ((1 << (SEMI_BITS_PER_BIN)) * FIX_B_PER_O) - distance;
            }

            if (distance > MAX_COMBINE_DISTANCE)
            {
                continue;
            }

            int into;
            int from;

            if (ed->note_peak_amps[i] > ed->note_peak_amps[j])
            {
                into = i;
                from = j;
            }
            else
            {
                into = j;
                from = i;
            }

            // We need to combine the notes.  We need to move the new note freq
            // towards the stronger of the two notes.
            int16_t amp1 = ed->note_peak_amps[into];
            int16_t amp2 = ed->note_peak_amps[from];

            // 0 to 32768 porportional to how much of amp1 we want.
            uint32_t porp = 0;
            if (amp1 || amp2)
            {
                porp = (amp1 << 15) / (amp1 + amp2);
            }
            uint16_t newnote = (nf1 * porp + nf2 * (32768 - porp)) >> 15;

            // When combining notes, we have to use the stronger amplitude note.
            // trying to average or combine the power of the notes looks awful.
            ed->note_peak_frequencies[into] = newnote;
            ed->note_peak_amps[into]  = (ed->note_peak_amps[into] > ed->note_peak_amps[from]) ? ed->note_peak_amps[into]
                                                                                              : ed->note_peak_amps[j];
            ed->note_peak_amps2[into] = (ed->note_peak_amps2[into] > ed->note_peak_amps2[from])
                                            ? ed->note_peak_amps2[into]
                                            : ed->note_peak_amps2[j];

            ed->note_peak_frequencies[from] = 255;
            ed->note_peak_amps[from]        = 0;
            ed->note_jumped_to[from]        = i;
        }

    // For al lof the notes that have not been hit, we have to allow them to
    // to decay.  We only do this for notes that have not found a peak.
    for (i = 0; i < MAX_NOTES; i++)
    {
        if (ed->note_peak_frequencies[i] == 255 || hitnotes[i])
        {
            continue;
        }

        ed->note_peak_amps[i] -= ed->note_peak_amps[i] >> AMP_1_IIR_BITS;
        ed->note_peak_amps2[i] -= ed->note_peak_amps2[i] >> AMP_2_IIR_BITS;

        // In the event a note is not strong enough anymore, it is to be
        // returned back into the great pool of unused notes.
        if (ed->note_peak_amps[i] < MINIMUM_AMP_FOR_NOTE_TO_DISAPPEAR)
        {
            ed->note_peak_frequencies[i] = 255;
            ed->note_peak_amps[i]        = 0;
            ed->note_peak_amps2[i]       = 0;
        }
    }

    // We now have notes!!!
#if 0
    for( i = 0; i < MAX_NOTES; i++ )
    {
        if( ed->note_peak_frequencies[i] == 255 )
        {
            continue;
        }
        printf( "(%3d %4d %4d) ", ed->note_peak_frequencies[i], ed->note_peak_amps[i], ed->note_peak_amps2[i] );
    }
    printf( "\n") ;
#endif

#if 0
    for( i = 0; i < FIX_B_PER_O; i++ )
    {
        printf( "%4d ", ed->folded_bins[i] );
    }
    printf( "\n" );
#endif
}
