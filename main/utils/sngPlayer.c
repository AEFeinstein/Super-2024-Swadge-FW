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

/** The maximum number of concurrent songs playable */
#define NUM_SONGS 2

/** The maximum number of oscillators per song */
#define OSC_PER_SONG 2

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A struct containing all state necessary to play a single song
 */
typedef struct
{
    synthOscillator_t oscillators[OSC_PER_SONG]; ///< The oscillators used to generate tones for the song
    int32_t samplesRemaining[OSC_PER_SONG]; ///< The number of samples remaining until the next note, per-oscillator
    int32_t cNoteIdx[OSC_PER_SONG];         ///< The current note index in the song, per-oscillator
    const song_t* song;                     ///< A pointer to the song being played
    bool songIsPlaying;                     ///< true if the song is being played, false if it is paused or stopped
    songFinishedCbFn songCb;                ///< An optional callback to call when the song is finished
} spkSong_t;

/**
 * @brief A struct containing all state necessary to play all songs
 */
typedef struct
{
    spkSong_t songStates[NUM_SONGS]; ///< An array of song states
} sngPlayer_t;

//==============================================================================
// Variables
//==============================================================================

/** The song player state */
static sngPlayer_t sp = {0};

/** An array of pointers to oscillators used for mixing samples */
static synthOscillator_t* oPtrs[NUM_SONGS * OSC_PER_SONG] = {0};

/** The default oscillator shapes */
const oscillatorShape_t oscShapes[OSC_PER_SONG] = {
    SHAPE_SQUARE,
    SHAPE_TRIANGLE,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the speaker song player
 */
void initSpkSongPlayer(void)
{
    // For each song
    for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
    {
        spkSong_t* s = &sp.songStates[sIdx];
        // Clear out state
        s->song          = NULL;
        s->songIsPlaying = false;
        s->songCb        = NULL;

        // For each oscillator in the song
        for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
        {
            // Clear out state
            s->cNoteIdx[oIdx]         = 0;
            s->samplesRemaining[oIdx] = 0;
            swSynthInitOscillator(&s->oscillators[oIdx], oscShapes[oIdx], 0, 0);

            // Save a pointer to the oscillator in the list of all oscillator pointers
            oPtrs[(sIdx * OSC_PER_SONG) + oIdx] = &s->oscillators[oIdx];
        }
    }
}

/**
 * @brief Play a song on the DAC speaker
 *
 * @param sIdx The index to play the song at, up to ::NUM_SONGS
 * @param song The song to play
 */
void spkSongPlay(uint8_t sIdx, const song_t* song)
{
    spkSongPlayCb(sIdx, song, NULL);
}

/**
 * @brief Play a song on the DAC speaker, and set up a callback to be called when the song finishes
 *
 * @param sIdx The index to play the song at, up to ::NUM_SONGS
 * @param song The song to play
 * @param cb The function pointer to call when the song is finished
 */
void spkSongPlayCb(uint8_t sIdx, const song_t* song, songFinishedCbFn cb)
{
    spkSong_t* s = &sp.songStates[sIdx];

    // Save this for later
    s->song          = song;
    s->songCb        = cb;
    s->songIsPlaying = true;

    // For each oscillator
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
            swSynthSetVolume(&s->oscillators[oIdx], SPK_MAX_VOLUME);
        }
    }
}

/**
 * @brief Stop playing all songs on the DAC speaker
 *
 * @param resetTracks true to clear the state for the songs, false to leave the state as-is
 */
void spkSongStop(bool resetTracks)
{
    // For each song
    for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
    {
        spkSong_t* s = &sp.songStates[sIdx];

        // If the the song is playing
        if (s->songIsPlaying)
        {
            // Stop it
            s->songIsPlaying = false;

            // NULL the song if it should be reset
            if (resetTracks)
            {
                s->song = NULL;
            }

            // For each oscillator
            for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
            {
                // Set the volume to zero
                swSynthSetVolume(&s->oscillators[oIdx], 0);

                // Clear the index and sample count if it should be reset
                if (resetTracks)
                {
                    s->cNoteIdx[oIdx]         = 0;
                    s->samplesRemaining[oIdx] = 0;
                }
            }
        }
    }
}

/**
 * @brief Pause all songs on the DAC speaker
 */
void spkSongPause(void)
{
    spkSongStop(false);
}

/**
 * @brief Resume playing all songs on the DAC speaker
 *
 */
void spkSongResume(void)
{
    // For each song
    for (int32_t sIdx = 0; sIdx < NUM_SONGS; sIdx++)
    {
        // Resume the song
        sp.songStates[sIdx].songIsPlaying = true;

        // For each oscillator
        for (int32_t oIdx = 0; oIdx < OSC_PER_SONG; oIdx++)
        {
            // Set the volume back
            swSynthSetVolume(&sp.songStates[sIdx].oscillators[oIdx], SPK_MAX_VOLUME);
        }
    }
}

/**
 * @brief Pause all songs on the DAC speaker and return a copy of the state, which may be restored later with
 * spkSongRestore()
 *
 * @return void* A pointer to the state. This must be either restored with spkSongRestore() or \c free()'d later,
 * otherwise the memory will leak
 */
void* spkSongSave(void)
{
    spkSongPause();
    void* savePtr = calloc(1, sizeof(sngPlayer_t));
    memcpy(savePtr, &sp, sizeof(sngPlayer_t));
    return savePtr;
}

/**
 * @brief Resume playing all songs on the DAC speaker with the given state
 *
 * @param data A pointer to the state from spkSongSave(). This memory will be \c free()'d
 */
void spkSongRestore(void* data)
{
    memcpy(&sp, data, sizeof(sngPlayer_t));
    free(data);
    spkSongResume();
}

/**
 * @brief Fill a buffer with the next set of samples for the currently playing song. This should be called by the
 * callback passed into initDac(), or used as the callback itself. Samples are generated at sampling rate of
 * ::AUDIO_SAMPLE_RATE_HZ
 *
 * @param samples An array of unsigned 8-bit samples to fill
 * @param len The length of the array to fill
 */
void sngPlayerFillBuffer(uint8_t* samples, int16_t len)
{
    // For each sample
    for (int32_t mIdx = 0; mIdx < len; mIdx++)
    {
        // Step and mix all the oscillators together
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

                        // Increment the note index, checking for wraparound
                        if (s->cNoteIdx[oIdx] == track->numNotes - 1)
                        {
                            // Song is over
                            if (s->song->shouldLoop)
                            {
                                // if it should loop, restart it
                                s->cNoteIdx[oIdx] = 0;
                            }
                            else
                            {
                                // Otherwise stop the song
                                s->songIsPlaying          = false;
                                s->cNoteIdx[oIdx]         = 0;
                                s->samplesRemaining[oIdx] = 0;
                                swSynthSetVolume(&s->oscillators[oIdx], 0);

                                // If there is a callback to call
                                if (NULL != s->songCb)
                                {
                                    // Call it, then clear it
                                    s->songCb();
                                    s->songCb = NULL;
                                }
                                continue;
                            }
                        }
                        else
                        {
                            // Simply increment to the next note
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
                            swSynthSetVolume(&s->oscillators[oIdx], SPK_MAX_VOLUME);
                        }
                    }
                }
            }
        }
    }
}
