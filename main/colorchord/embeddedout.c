//Copyright 2015 <>< Charles Lohr under the ColorChord License.

//==============================================================================
// Includes
//==============================================================================

#include "embeddedout.h"
#include "color_utils.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param eod
 * @param end
 */
void UpdateLinearLEDs(embeddedout_data* eod, embeddednf_data* end)
{
    //Source material:
    /*
        extern uint8_t  end->note_peak_freqs[];
        extern uint16_t end->note_peak_amps[];  //[MAXNOTES]
        extern uint16_t end->note_peak_amps2[];  //[MAXNOTES]  (Responds quicker)
        extern uint8_t  end->note_jumped_to[]; //[MAXNOTES] When a note combines into another one,
    */

    //Goal: Make splotches of light that are porportional to the strength of notes.
    //Color them according to value in end->note_peak_amps2.

    uint8_t i;
    uint16_t j, l;
    uint32_t total_size_all_notes = 0;
    int32_t porpamps[MAXNOTES]; //LEDs for each corresponding note.
    uint8_t sorted_note_map[MAXNOTES]; //mapping from which note into the array of notes from the rest of the system.
    uint8_t sorted_map_count = 0;
    uint32_t note_nerf_a = 0;

    for( i = 0; i < MAXNOTES; i++ )
    {
        if( end->note_peak_freqs[i] == 255 )
        {
            continue;
        }
        note_nerf_a += end->note_peak_amps[i];
    }

    note_nerf_a = ((note_nerf_a * NERF_NOTE_PORP) >> 8);


    for( i = 0; i < MAXNOTES; i++ )
    {
        uint16_t ist = end->note_peak_amps[i];
        uint8_t nff = end->note_peak_freqs[i];
        if( nff == 255 )
        {
            continue;
        }
        if( ist < note_nerf_a )
        {
            continue;
        }

#if SORT_NOTES
        for( j = 0; j < sorted_map_count; j++ )
        {
            if( end->note_peak_freqs[ sorted_note_map[j] ] > nff )
            {
                break; // so j is correct place to insert
            }
        }
        for( k = sorted_map_count; k > j; k-- ) // make room
        {
            sorted_note_map[k] = sorted_note_map[k - 1];
        }
        sorted_note_map[j] = i; // insert in correct place
#else
        sorted_note_map[sorted_map_count] = i; // insert at end
#endif
        sorted_map_count++;
    }

#if 0
    for( i = 0; i < sorted_map_count; i++ )
    {
        printf( "%d: %d: %d /", sorted_note_map[i],  end->note_peak_freqs[sorted_note_map[i]],
                end->note_peak_amps[sorted_note_map[i]] );
    }
    printf( "\n" );
#endif

    uint16_t local_peak_amps[MAXNOTES];
    uint16_t local_peak_amps2[MAXNOTES];
    uint8_t  local_peak_freq[MAXNOTES];

    //Make a copy of all of the variables into local ones so we don't have to keep double-dereferencing.
    for( i = 0; i < sorted_map_count; i++ )
    {
        //printf( "%5d ", local_peak_amps[i] );
        local_peak_amps[i] = end->note_peak_amps[sorted_note_map[i]] - note_nerf_a;
        local_peak_amps2[i] = end->note_peak_amps2[sorted_note_map[i]];
        local_peak_freq[i] = end->note_peak_freqs[sorted_note_map[i]];
        //        printf( "%5d ", local_peak_amps[i] );
    }
    //    printf( "\n" );

    for( i = 0; i < sorted_map_count; i++ )
    {
        porpamps[i] = 0;
        total_size_all_notes += local_peak_amps[i];
    }

    if( total_size_all_notes == 0 )
    {
        for( j = 0; j < CONFIG_NUM_LEDS * 3; j++ )
        {
            eod->ledOut[j] = 0;
        }
        return;
    }

    uint32_t porportional = (uint32_t)(CONFIG_NUM_LEDS << 16) / ((uint32_t)total_size_all_notes);

    uint16_t total_accounted_leds = 0;

    for( i = 0; i < sorted_map_count; i++ )
    {
        porpamps[i] = (local_peak_amps[i] * porportional) >> 16;
        total_accounted_leds += porpamps[i];
    }

    int16_t total_unaccounted_leds = CONFIG_NUM_LEDS - total_accounted_leds;

    int addedlast = 1;
    do
    {
        for( i = 0; i < sorted_map_count && total_unaccounted_leds; i++ )
        {
            porpamps[i]++;
            total_unaccounted_leds--;
            addedlast = 1;
        }
    } while( addedlast && total_unaccounted_leds );

    //Put the frequencies on a ring.
    j = 0;
    for( i = 0; i < sorted_map_count; i++ )
    {
        while( porpamps[i] > 0 )
        {
            eod->ledFreqOut[j] = local_peak_freq[i];
            eod->ledAmpOut[j] = (local_peak_amps2[i] * NOTE_FINAL_AMP) >> 8;
            j++;
            porpamps[i]--;
        }
    }

    //This part totally can't run on an embedded system.
#if LIN_WRAPAROUND
    uint16_t midx = 0;
    uint32_t mqty = 100000000;
    for( j = 0; j < USE_CONFIG_NUM_LEDS; j++ )
    {
        uint32_t dqty;
        uint16_t localj;

        dqty = 0;
        localj = j;
        for( l = 0; l < USE_CONFIG_NUM_LEDS; l++ )
        {
            int32_t d = (int32_t)eod->ledFreqOut[localj] - (int32_t)eod->ledFreqOutOld[l];
            if( d < 0 )
            {
                d *= -1;
            }
            if( d > (NOTERANGE >> 1) )
            {
                d = NOTERANGE - d + 1;
            }
            dqty += ( d * d );

            localj++;
            if( localj == USE_CONFIG_NUM_LEDS )
            {
                localj = 0;
            }
        }

        if( dqty < mqty )
        {
            mqty = dqty;
            midx = j;
        }
    }

    eod->ledSpin = midx;

#else
    eod->ledSpin = 0;
#endif

    j = eod->ledSpin;
    for( l = 0; l < CONFIG_NUM_LEDS; l++, j++ )
    {
        if( j >= CONFIG_NUM_LEDS )
        {
            j = 0;
        }
        eod->ledFreqOutOld[l] = eod->ledFreqOut[j];

        uint16_t amp = eod->ledAmpOut[j];
        if( amp > 255 )
        {
            amp = 255;
        }
        uint32_t color = ECCtoHEX( (eod->ledFreqOut[j] + eod->RootNoteOffset) % NOTERANGE, 255, amp );

        // Flipping red and green here.
        eod->ledOut[l * 3 + 1] = ( color >> 0 ) & 0xff;
        eod->ledOut[l * 3 + 0] = ( color >> 8 ) & 0xff;
        eod->ledOut[l * 3 + 2] = ( color >> 16 ) & 0xff;
    }
    /*    j = eod->ledSpin;
        for( i = 0; i < sorted_map_count; i++ )
        {
            while( porpamps[i] > 0 )
            {
                uint16_t amp = ((uint32_t)local_peak_amps2[i] * NOTE_FINAL_AMP) >> 8;
                if( amp > 255 ) amp = 255;
                uint32_t color = ECCtoHEX( local_peak_freq[i], 255, amp );
                eod->ledOut[j*3+0] = ( color >> 0 ) & 0xff;
                eod->ledOut[j*3+1] = ( color >> 8 ) & 0xff;
                eod->ledOut[j*3+2] = ( color >>16 ) & 0xff;

                j++;
                if( j == USE_CONFIG_NUM_LEDS ) j = 0;
                porpamps[i]--;
            }
        }*/

    //Now, we use porpamps to march through the LEDs, coloring them.
    /*    j = 0;
        for( i = 0; i < sorted_map_count; i++ )
        {
            while( porpamps[i] > 0 )
            {
                uint16_t amp = ((uint32_t)local_peak_amps2[i] * NOTE_FINAL_AMP) >> 8;
                if( amp > 255 ) amp = 255;
                uint32_t color = ECCtoHEX( local_peak_freq[i], 255, amp );
                eod->ledOut[j*3+0] = ( color >> 0 ) & 0xff;
                eod->ledOut[j*3+1] = ( color >> 8 ) & 0xff;
                eod->ledOut[j*3+2] = ( color >>16 ) & 0xff;

                j++;
                porpamps[i]--;
            }
        }*/
}

/**
 * @brief TODO
 *
 * @param eod
 * @param end
 */
void UpdateAllSameLEDs(embeddedout_data* eod, embeddednf_data* end)
{
    int i;
    uint8_t freq = 0;
    uint16_t amp = 0;

    for( i = 0; i < MAXNOTES; i++ )
    {
        uint16_t ist = end->note_peak_amps2[i];
        uint8_t ifrq = end->note_peak_freqs[i];
        if( ist > amp && ifrq != 255 )
        {
            freq = ifrq;
            amp = ist;
        }
    }

    amp = (((uint32_t)(amp)) * NOTE_FINAL_AMP) >> 10;

    if( amp > 255 )
    {
        amp = 255;
    }
    uint32_t color = ECCtoHEX( (freq + eod->RootNoteOffset) % NOTERANGE, 255, amp );

    for( i = 0; i < CONFIG_NUM_LEDS; i++ )
    {
        // Flipping red and green, here.
        eod->ledOut[i * 3 + 1] = ( color >> 0 ) & 0xff;
        eod->ledOut[i * 3 + 0] = ( color >> 8 ) & 0xff;
        eod->ledOut[i * 3 + 2] = ( color >> 16 ) & 0xff;
    }
}

/**
 * @brief TODO
 *
 * @param note
 * @param sat
 * @param val
 * @return uint32_t
 */
uint32_t ECCtoHEX( uint8_t note, uint8_t sat, uint8_t val )
{
    uint16_t hue = 0;
    uint16_t third = 65535 / 3;
    uint32_t renote = ((uint32_t)note * 65536) / NOTERANGE;

    //Note is expected to be a vale from 0..(NOTERANGE-1)
    //renote goes from 0 to the next one under 65536.


    if( renote < third )
    {
        //Yellow to Red.
        hue = (third - renote) >> 1;
    }
    else if( renote < (uint16_t)(third << 1) )
    {
        //Red to Blue
        hue = (third - renote);
    }
    else
    {
        //hue = ((((65535-renote)>>8) * (uint32_t)(third>>8)) >> 1) + (third<<1);
        hue = (uint16_t)(((uint32_t)(65536 - renote) << 16) / (third << 1)) + (third >>
                1); // ((((65535-renote)>>8) * (uint32_t)(third>>8)) >> 1) + (third<<1);
    }
    hue >>= 8;

    return EHSVtoHEXhelper(hue, sat, val, true);
}
