//==============================================================================
// Includes
//==============================================================================

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "hdw-dac.h"
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
    synthOscillator_t oscillators[OSC_PER_SONG];
    int32_t samplesRemaining[OSC_PER_SONG];
    int32_t cNoteIdx[OSC_PER_SONG];
    const song_t* song;
    bool songIsPlaying;
    songFinishedCbFn songCb;
} spkSong_t;

typedef struct
{
    spkSong_t songStates[NUM_SONGS];
} sngPlayer_t;

//==============================================================================
// Variables
//==============================================================================

sngPlayer_t sp = {0};
synthOscillator_t* oPtrs[NUM_SONGS * OSC_PER_SONG];

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
        spkSong_t* s     = &sp.songStates[sIdx];
        s->song          = NULL;
        s->songIsPlaying = false;
        s->songCb        = NULL;

        for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
        {
            s->cNoteIdx[oIdx]         = 0;
            s->samplesRemaining[oIdx] = 0;
            swSynthInitOscillator(&s->oscillators[oIdx], oscShapes[oIdx], 0, 0);

            oPtrs[(sIdx * OSC_PER_SONG) + oIdx] = &s->oscillators[oIdx];
        }
    }
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
    spkSong_t* s = &sp.songStates[sIdx];
    // Save this for later
    s->song          = song;
    s->songCb        = cb;
    s->songIsPlaying = true;

    for (int oIdx = 0; oIdx < song->numTracks; oIdx++)
    {
        // Reset the noteIdx
        s->cNoteIdx[oIdx] = 0;

        // Get the first note for this track
        musicalNote_t* note = &song->tracks[oIdx].notes[0];
        // Calculate the number of samples for this note
        s->samplesRemaining[oIdx] = (note->timeMs * AUDIO_SAMPLE_RATE_HZ) / 1000;
        // Play the note
        if (SILENCE == note->note)
        {
            swSynthSetVolume(&s->oscillators[oIdx], 0);
        }
        else
        {
            swSynthSetFreq(&s->oscillators[oIdx], note->note);
            swSynthSetVolume(&s->oscillators[oIdx], 255);
        }
    }
}

/**
 * @brief TODO
 *
 * @param resetTracks
 */
void spkSongStop(bool resetTracks)
{
    for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
    {
        spkSong_t* s = &sp.songStates[sIdx];
        if (s->songIsPlaying)
        {
            s->songIsPlaying = false;
            if (resetTracks)
            {
                s->song = NULL;
            }
            for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
            {
                if (resetTracks)
                {
                    s->cNoteIdx[oIdx]         = 0;
                    s->samplesRemaining[oIdx] = 0;
                }
                swSynthSetVolume(&s->oscillators[oIdx], 0);
            }
        }
    }
}

/**
 * @brief TODO
 *
 */
void spkSongPause(void)
{
    // Set the volume to zero
    for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
    {
        // Pause the song
        sp.songStates[sIdx].songIsPlaying = false;
        for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
        {
            swSynthSetVolume(&sp.songStates[sIdx].oscillators[oIdx], 0);
        }
    }
}

/**
 * @brief
 *
 */
void spkSongResume(void)
{
    // Set the volume to non-zero
    for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
    {
        // Resume the song
        sp.songStates[sIdx].songIsPlaying = true;
        for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
        {
            swSynthSetVolume(&sp.songStates[sIdx].oscillators[oIdx], 255);
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
        samples[mIdx] = swSynthMixOscillators(oPtrs, NUM_SONGS * OSC_PER_SONG);

        // For each song
        for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
        {
            spkSong_t* s = &sp.songStates[sIdx];

            // If there is a song to play
            if (NULL != s->song && s->songIsPlaying)
            {
                // For each track
                for (int32_t oIdx = 0; oIdx < s->song->numTracks; oIdx++)
                {
                    // Decrement samples remaining
                    s->samplesRemaining[oIdx]--;
                    // If it's time for the next note
                    if (0 == s->samplesRemaining[oIdx])
                    {
                        // Get the song's track
                        songTrack_t* track = &s->song->tracks[oIdx];

                        // Increment the note index
                        if (s->cNoteIdx[oIdx] == track->numNotes - 1)
                        {
                            if (s->song->shouldLoop)
                            {
                                s->cNoteIdx[oIdx] = 0;
                            }
                            else
                            {
                                // Stop the song
                                s->songIsPlaying          = false;
                                s->cNoteIdx[oIdx]         = 0;
                                s->samplesRemaining[oIdx] = 0;
                                swSynthSetVolume(&s->oscillators[oIdx], 0);

                                if (NULL != s->songCb)
                                {
                                    s->songCb();
                                    s->songCb = NULL;
                                }
                                continue;
                            }
                        }
                        else
                        {
                            s->cNoteIdx[oIdx]++;
                        }

                        // Get the note
                        musicalNote_t* note = &track->notes[s->cNoteIdx[oIdx]];

                        // Calculate remaining samples
                        s->samplesRemaining[oIdx] = (note->timeMs * AUDIO_SAMPLE_RATE_HZ) / 1000;

                        // Play the note
                        if (SILENCE == note->note)
                        {
                            swSynthSetVolume(&s->oscillators[oIdx], 0);
                        }
                        else
                        {
                            swSynthSetFreq(&s->oscillators[oIdx], note->note);
                            swSynthSetVolume(&s->oscillators[oIdx], 255);
                        }
                    }
                }
            }
        }
    }
}
