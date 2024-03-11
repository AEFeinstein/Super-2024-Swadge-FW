//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include "sngPlayer.h"
#include "macros.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_SONGS    2
#define OSC_PER_SONG 2

//==============================================================================
// Variables
//==============================================================================

synthOscillator_t oscillators[NUM_SONGS * OSC_PER_SONG];
int32_t samplesRemaining[NUM_SONGS * OSC_PER_SONG];
int32_t cNoteIdx[NUM_SONGS * OSC_PER_SONG];
song_t* songs[NUM_SONGS];

const oscillatorShape_t oscShapes[OSC_PER_SONG] = {
    SHAPE_SQUARE,
    SHAPE_TRIANGLE,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 */
void initSpkSongPlayer(void)
{
    for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
    {
        songs[sIdx] = NULL;

        for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
        {
            uint32_t idx          = sIdx * OSC_PER_SONG + oIdx;
            cNoteIdx[idx]         = -1;
            samplesRemaining[idx] = 0;
            swSynthInitOscillator(&oscillators[idx], oscShapes[oIdx], 0);
        }
    }
}

/**
 * @brief TODO
 *
 * @param sIdx
 * @param song
 */
void spkPlaySong(uint8_t sIdx, song_t* song)
{
    printf("%s::%d\n", __func__, __LINE__);
    // Save this for later
    songs[sIdx] = song;

    for (int oIdx = 0; oIdx < song->numTracks; oIdx++)
    {
        uint32_t idx = sIdx * OSC_PER_SONG + oIdx;

        // Reset the noteIdx
        cNoteIdx[idx] = 0;

        // Get the first note for this track
        musicalNote_t* note = &song->tracks[oIdx].notes[0];
        // Calculate the number of samples for this note
        samplesRemaining[idx] = (note->timeMs * AUDIO_SAMPLE_RATE_HZ) / 1000;
        // Play the note
        if (SILENCE == note->note)
        {
            swSynthSetVolume(&oscillators[idx], 0);
        }
        else
        {
            swSynthSetFreq(&oscillators[idx], note->note);
            swSynthSetVolume(&oscillators[idx], 255);
        }
    }
}

/**
 * @brief TODO
 *
 * @param samples
 * @param len
 */
void sngPlayerFillBuffer(uint8_t* samples, int16_t len)
{
    // For each sample
    for (int32_t mIdx = 0; mIdx < len; mIdx++)
    {
        // Mix all the oscillators together
        samples[mIdx] = swSynthMixOscillators(oscillators, ARRAY_SIZE(oscillators));

        // For each song
        for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
        {
            // If there is a song to play
            if (NULL != songs[sIdx])
            {
                // For each track
                for (int32_t oIdx = 0; oIdx < songs[sIdx]->numTracks; oIdx++)
                {
                    uint32_t idx = sIdx * OSC_PER_SONG + oIdx;

                    if (-1 != cNoteIdx[idx])
                    {
                        // Decrement samples remaining
                        samplesRemaining[idx]--;

                        // If it's time for the next note
                        if (0 == samplesRemaining[idx])
                        {
                            // Get the song's track
                            songTrack_t* track = &songs[sIdx]->tracks[oIdx];

                            // Increment the note index
                            if (cNoteIdx[idx] == track->numNotes - 1)
                            {
                                if (songs[sIdx]->shouldLoop)
                                {
                                    cNoteIdx[idx] = 0;
                                }
                                else
                                {
                                    // Stop the song
                                    cNoteIdx[idx] = -1;
                                    swSynthSetVolume(&oscillators[idx], 0);
                                    swSynthSetFreq(&oscillators[idx], 0);
                                    continue;
                                }
                            }
                            else
                            {
                                cNoteIdx[idx]++;
                            }

                            // Get the note
                            musicalNote_t* note = &track->notes[cNoteIdx[idx]];

                            // Calculate remaining samples
                            samplesRemaining[idx] = (note->timeMs * AUDIO_SAMPLE_RATE_HZ) / 1000;

                            // Play the note
                            if (SILENCE == note->note)
                            {
                                swSynthSetVolume(&oscillators[idx], 0);
                            }
                            else
                            {
                                swSynthSetFreq(&oscillators[idx], note->note);
                                swSynthSetVolume(&oscillators[idx], 255);
                            }
                        }
                    }
                }
            }
        }
    }
}