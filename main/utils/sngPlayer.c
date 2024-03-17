//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "sngPlayer.h"
#include "macros.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_SONGS    2
#define OSC_PER_SONG 2

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    synthOscillator_t oscillators[NUM_SONGS * OSC_PER_SONG];
    int32_t samplesRemaining[NUM_SONGS * OSC_PER_SONG];
    int32_t cNoteIdx[NUM_SONGS * OSC_PER_SONG];
    const song_t* songs[NUM_SONGS];
    bool songIsPlaying;
    songFinishedCbFn songCb; // TODO should be per-song
} sngPlayer_t;

//==============================================================================
// Variables
//==============================================================================

sngPlayer_t sp = {.songIsPlaying = false};

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
        sp.songs[sIdx] = NULL;

        for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
        {
            uint32_t idx             = sIdx * OSC_PER_SONG + oIdx;
            sp.cNoteIdx[idx]         = 0;
            sp.samplesRemaining[idx] = 0;
            swSynthInitOscillator(&sp.oscillators[idx], oscShapes[oIdx], 0);
        }
    }
    sp.songIsPlaying = false;
    sp.songCb        = NULL;
}

/**
 * @brief TODO
 *
 * @param sIdx
 * @param song
 */
void spkSongPlay(uint8_t sIdx, const song_t* song)
{
    spkSongPlayCb(sIdx, song, NULL);
}

/**
 * @brief TODO
 *
 * @param sIdx
 * @param song
 * @param cb
 */
void spkSongPlayCb(uint8_t sIdx, const song_t* song, songFinishedCbFn cb)
{
    // Save this for later
    sp.songs[sIdx] = song;
    sp.songCb      = cb;

    for (int oIdx = 0; oIdx < song->numTracks; oIdx++)
    {
        uint32_t idx = sIdx * OSC_PER_SONG + oIdx;

        // Reset the noteIdx
        sp.cNoteIdx[idx] = 0;

        // Get the first note for this track
        musicalNote_t* note = &song->tracks[oIdx].notes[0];
        // Calculate the number of samples for this note
        sp.samplesRemaining[idx] = (note->timeMs * AUDIO_SAMPLE_RATE_HZ) / 1000;
        // Play the note
        if (SILENCE == note->note)
        {
            swSynthSetVolume(&sp.oscillators[idx], 0);
        }
        else
        {
            swSynthSetFreq(&sp.oscillators[idx], note->note);
            swSynthSetVolume(&sp.oscillators[idx], 255);
        }
    }
    sp.songIsPlaying = true;
}

/**
 * @brief TODO
 *
 * @param resetTracks
 */
void spkSongStop(bool resetTracks)
{
    if (sp.songIsPlaying)
    {
        sp.songIsPlaying = false;
        for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
        {
            if (resetTracks)
            {
                sp.songs[sIdx] = NULL;
            }
            for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
            {
                uint32_t idx = sIdx * OSC_PER_SONG + oIdx;
                if (resetTracks)
                {
                    sp.cNoteIdx[idx]         = 0;
                    sp.samplesRemaining[idx] = 0;
                }
                swSynthSetVolume(&sp.oscillators[idx], 0);
            }
        }
        if (sp.songCb)
        {
            sp.songCb();
        }
    }
}

/**
 * @brief TODO
 *
 */
void spkSongPause(void)
{
    // Pause the song
    sp.songIsPlaying = false;

    // Set the volume to zero
    for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
    {
        for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
        {
            uint32_t idx = sIdx * OSC_PER_SONG + oIdx;
            swSynthSetVolume(&sp.oscillators[idx], 0);
        }
    }
}

/**
 * @brief
 *
 */
void spkSongResume(void)
{
    // Resume the song
    sp.songIsPlaying = true;

    // Set the volume to non-zero
    for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
    {
        for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
        {
            uint32_t idx = sIdx * OSC_PER_SONG + oIdx;
            swSynthSetVolume(&sp.oscillators[idx], 255);
        }
    }
}

/**
 * @brief TODO
 *
 * @return void*
 */
void* spkSongSave(void)
{
    spkSongPause();
    void* savePtr = calloc(1, sizeof(sngPlayer_t));
    memcpy(savePtr, &sp, sizeof(sngPlayer_t));
    return savePtr;
}

/**
 * @brief TODO
 *
 * @param data
 */
void spkSongRestore(void* data)
{
    memcpy(&sp, data, sizeof(sngPlayer_t));
    free(data);
    spkSongResume();
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
        samples[mIdx] = swSynthMixOscillators(sp.oscillators, ARRAY_SIZE(sp.oscillators));

        // For each song
        for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
        {
            // If there is a song to play
            if (NULL != sp.songs[sIdx])
            {
                // For each track
                for (int32_t oIdx = 0; oIdx < sp.songs[sIdx]->numTracks; oIdx++)
                {
                    uint32_t idx = sIdx * OSC_PER_SONG + oIdx;

                    if (sp.songIsPlaying)
                    {
                        // Decrement samples remaining
                        sp.samplesRemaining[idx]--;
                        // If it's time for the next note
                        if (0 == sp.samplesRemaining[idx])
                        {
                            // Get the song's track
                            songTrack_t* track = &sp.songs[sIdx]->tracks[oIdx];

                            // Increment the note index
                            if (sp.cNoteIdx[idx] == track->numNotes - 1)
                            {
                                if (sp.songs[sIdx]->shouldLoop)
                                {
                                    sp.cNoteIdx[idx] = 0;
                                }
                                else
                                {
                                    // Stop the song
                                    spkSongStop(false);
                                    continue;
                                }
                            }
                            else
                            {
                                sp.cNoteIdx[idx]++;
                            }

                            // Get the note
                            musicalNote_t* note = &track->notes[sp.cNoteIdx[idx]];

                            // Calculate remaining samples
                            sp.samplesRemaining[idx] = (note->timeMs * AUDIO_SAMPLE_RATE_HZ) / 1000;

                            // Play the note
                            if (SILENCE == note->note)
                            {
                                swSynthSetVolume(&sp.oscillators[idx], 0);
                            }
                            else
                            {
                                swSynthSetFreq(&sp.oscillators[idx], note->note);
                                swSynthSetVolume(&sp.oscillators[idx], 255);
                            }
                        }
                    }
                }
            }
        }
    }
}
