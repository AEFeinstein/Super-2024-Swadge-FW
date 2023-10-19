//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include <math.h>

#include <esp_timer.h>
#include "sound.h"
#include "hdw-bzr.h"
#include "hdw-mic_emu.h"
#include "emu_main.h"

//==============================================================================
// Defines
//==============================================================================

#define SAMPLING_RATE 8000

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A buzzer track which a song is played on, either BGM or SFX
 */
typedef struct
{
    const songTrack_t* sTrack; ///< The song currently being played on this track
    int32_t note_index;        ///< The note index into the song
    int32_t usAccum;           ///< Accumulated time for the current note
    bool should_loop;          ///< True if this track should loop, false if it plays once
} bzrTrack_t;

/**
 * @brief A buzzer, currently either left or right
 */
typedef struct
{
    noteFrequency_t cFreq; ///< The current frequency of the note being played
    uint16_t vol;          ///< The current volume
    bzrTrack_t bgm;        ///< The BGM track for this buzzer
    bzrTrack_t sfx;        ///< The SFX track for this buzzer
} buzzer_t;

//==============================================================================
// Const variables
//==============================================================================

const uint16_t volLevels[] = {
    0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
};

//==============================================================================
// Variables
//==============================================================================

/// The sound driver
static struct SoundDriver* soundDriver = NULL;

// Output buzzers
static buzzer_t buzzers[NUM_BUZZERS] = {0};
uint16_t bgmVolume;
uint16_t sfxVolume;

/// @brief Track if the buzzer is paused or not
static bool bzrPaused = false;

//==============================================================================
// Function Prototypes
//==============================================================================

static bool buzzer_track_check_next_note(bzrTrack_t* track, int16_t bIdx, uint16_t volume, bool isActive,
                                         int32_t tElapsedUs);
void buzzer_check_next_note(void* arg);
void EmuSoundCb(struct SoundDriver* sd, short* in, short* out, int samples_R, int samples_W);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the buzzer
 *
 * @param bzrGpioL The GPIO the left buzzer is attached to
 * @param ledcTimerL The LEDC timer used to drive the left buzzer
 * @param ledcChannelL The LEDC channel used to drive the left buzzer
 * @param bzrGpioR The GPIO the right buzzer is attached to
 * @param ledcTimerR The LEDC timer used to drive the right buzzer
 * @param ledcChannelR The LEDC channel used to drive the right buzzer
 * @param bgmVolume Starting background sound volume, 0 to 4096
 * @param sfxVolume Starting effects sound volume, 0 to 4096
 */
void initBuzzer(gpio_num_t bzrGpioL, ledc_timer_t ledcTimerL, ledc_channel_t ledcChannelL, gpio_num_t bzrGpioR,
                ledc_timer_t ledcTimerR, ledc_channel_t ledcChannelR, uint16_t _bgmVolume, uint16_t _sfxVolume)
{
    bzrStop(true);

    if (!soundDriver)
    {
        soundDriver = InitSound(0, EmuSoundCb, SAMPLING_RATE, 1, 2, 256, 0, 0);
    }

    memset(&buzzers, 0, sizeof(buzzers));
    bzrSetBgmVolume(_bgmVolume);
    bzrSetSfxVolume(_sfxVolume);

    const esp_timer_create_args_t checkNoteTimeArgs = {
        .arg                   = NULL,
        .callback              = buzzer_check_next_note,
        .dispatch_method       = ESP_TIMER_TASK,
        .name                  = "BZR",
        .skip_unhandled_events = true,
    };
    esp_timer_handle_t checkNoteTimerHandle = NULL;
    esp_timer_create(&checkNoteTimeArgs, &checkNoteTimerHandle);
    esp_timer_start_periodic(checkNoteTimerHandle, 1);
}

/**
 * @brief Deinitialize the buzzer
 */
void deinitBuzzer(void)
{
    if (soundDriver)
    {
#if defined(_WIN32) || defined(__CYGWIN__)
        CloseSound(NULL);
#else
        CloseSound(soundDriver); // when calling this on Windows, it halts
#endif
        soundDriver = NULL;
    }
}

/**
 * @brief Set the buzzer's bgm volume
 *
 * @param vol The background volume, 0 to MAX_VOLUME
 */
void bzrSetBgmVolume(uint16_t vol)
{
    bgmVolume = volLevels[vol];
}

/**
 * @brief Set the buzzer's sfx volume
 *
 * @param vol The background volume, 0 to MAX_VOLUME
 */
void bzrSetSfxVolume(uint16_t vol)
{
    sfxVolume = volLevels[vol];
}

/**
 * @brief Play a song_t on one or two bzrTrack_t depending on what
 *
 * @param trackL The left track to play on
 * @param trackR The right track to play on
 * @param song The song_t to play
 * @param track The requested track or tracks to play on
 */
static void bzrPlayTrack(bzrTrack_t* trackL, bzrTrack_t* trackR, const song_t* song, buzzerPlayTrack_t track)
{
    if (1 == song->numTracks)
    {
        // Mono song, play it on the requested tracks
        if (BZR_STEREO == track || BZR_LEFT == track)
        {
            trackL->sTrack      = &song->tracks[0];
            trackL->note_index  = -1;
            trackL->usAccum     = 0;
            trackL->should_loop = song->shouldLoop;
        }

        if (BZR_STEREO == track || BZR_RIGHT == track)
        {
            trackR->sTrack      = &song->tracks[0];
            trackR->note_index  = -1;
            trackR->usAccum     = 0;
            trackL->should_loop = song->shouldLoop;
        }
    }
    else
    {
        // Stereo song, play it on both tracks
        trackL->sTrack      = &song->tracks[0];
        trackL->note_index  = -1;
        trackL->usAccum     = 0;
        trackL->should_loop = song->shouldLoop;

        trackR->sTrack      = &song->tracks[1];
        trackR->note_index  = -1;
        trackR->usAccum     = 0;
        trackR->should_loop = song->shouldLoop;
    }
}

/**
 * @brief Start playing a background music on the buzzer. This has lower priority
 * than sound effects
 *
 * @param song The song to play as a sequence of notes
 * @param track The track to play BGM on
 */
void bzrPlayBgm(const song_t* song, buzzerPlayTrack_t track)
{
    bzrPlayTrack(&buzzers[0].bgm, &buzzers[1].bgm, song, track);
}

/**
 * @brief Start playing a sound effect on the buzzer. This has higher priority
 * than background music
 *
 * @param song The song to play as a sequence of notes
 * @param track The track to play SFX on
 */
void bzrPlaySfx(const song_t* song, buzzerPlayTrack_t track)
{
    bzrPlayTrack(&buzzers[0].sfx, &buzzers[1].sfx, song, track);
}

/**
 * @brief Stop the buzzer from playing anything
 * @param resetTracks true to reset track data as well
 */
void bzrStop(bool resetTracks)
{
    if (resetTracks)
    {
        memset(buzzers, 0, sizeof(buzzers));
    }
    bzrPlayNote(SILENCE, BZR_LEFT, 0);
    bzrPlayNote(SILENCE, BZR_RIGHT, 0);
}

/////////////////////////////

/**
 * @brief Start playing a single note on the buzzer.
 * This note will play until stopped.
 * This has IRAM_ATTR because it may be called from an interrupt
 *
 * @param freq The frequency of the note
 * @param track The track to play a note on
 * @param volume The volume, 0 to 4096
 */
void bzrPlayNote(noteFrequency_t freq, buzzerPlayTrack_t track, uint16_t volume)
{
    switch (track)
    {
        case BZR_STEREO:
        {
            bzrPlayNote(freq, BZR_LEFT, volume);
            bzrPlayNote(freq, BZR_RIGHT, volume);
            break;
        }
        case BZR_LEFT:
        case BZR_RIGHT:
        {
            buzzers[track].cFreq = freq;
            buzzers[track].vol   = volume;
            break;
        }
    }
}

/**
 * @brief Stop playing a single note on the buzzer
 * This has IRAM_ATTR because it may be called from an interrupt
 *
 * @param track The track to stop
 */
void bzrStopNote(buzzerPlayTrack_t track)
{
    bzrPlayNote(SILENCE, track, 0);
}

////////////////////////////////

/**
 * @brief Call this periodically to check if the next note in the song should be played
 *
 * @param arg unused
 */
void buzzer_check_next_note(void* arg)
{
    static int32_t tLastLoopUs = 0;

    if (0 == tLastLoopUs)
    {
        tLastLoopUs = esp_timer_get_time();
    }
    else
    {
        int32_t tNowUs     = esp_timer_get_time();
        int32_t tElapsedUs = tNowUs - tLastLoopUs;
        tLastLoopUs        = tNowUs;

        // If paused, return here so tElapsedUs stays sane
        if (bzrPaused)
        {
            return;
        }

        for (int16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
        {
            buzzer_t* buzzer = &buzzers[bIdx];

            bool sfxIsActive = buzzer_track_check_next_note(&buzzer->sfx, bIdx, sfxVolume, true, tElapsedUs);
            bool bgmIsActive = buzzer_track_check_next_note(&buzzer->bgm, bIdx, bgmVolume, !sfxIsActive, tElapsedUs);

            // If nothing is playing, but there is BGM (i.e. SFX finished)
            if ((false == sfxIsActive) && (false == bgmIsActive) && (NULL != buzzer->bgm.sTrack))
            {
                // Immediately start playing BGM to get back on track faster
                bzrPlayNote(buzzer->bgm.sTrack->notes[buzzer->bgm.note_index].note, bIdx, bgmVolume);
            }
        }
    }
}

/**
 * @brief Advance the notes in a specific track and play them if the track is active
 *
 * @param track The track to check notes in
 * @param bIdx The index of the buzzer to play on
 * @param volume The volume to play at
 * @param isActive true if this is active and should set a note to be played
 *                 false to just advance notes without playing
 * @param tElapsedUs The microseconds since this function was last called
 * @return true  if this track is playing a note
 *         false if it is not
 */
static bool buzzer_track_check_next_note(bzrTrack_t* track, int16_t bIdx, uint16_t volume, bool isActive,
                                         int32_t tElapsedUs)
{
    // Check if there is a song and there are still notes
    if ((NULL != track->sTrack) && (track->note_index < track->sTrack->numNotes))
    {
        // Check if it's time to play the next note
        bool shouldAdvance = false;
        if (-1 == track->note_index)
        {
            track->note_index++;
            track->usAccum = 0;
            shouldAdvance  = true;
        }
        else
        {
            // Accumulate time
            track->usAccum += tElapsedUs;
            // Get this note's length in microseconds
            int32_t noteTimeUs = (1000 * track->sTrack->notes[track->note_index].timeMs);
            // If we've accumulated as much time as the note
            if (track->usAccum >= noteTimeUs)
            {
                // Decrement the accumulated time
                track->usAccum -= noteTimeUs;
                // Advance to the next note
                track->note_index++;
                shouldAdvance = true;
            }
        }

        // Check if it's time to play the next note
        if (shouldAdvance)
        {
            // Loop if requested
            if (track->should_loop && (track->note_index == track->sTrack->numNotes))
            {
                track->note_index = track->sTrack->loopStartNote;
            }

            // If there is a note
            if (track->note_index < track->sTrack->numNotes)
            {
                if (isActive)
                {
                    // Play the note
                    bzrPlayNote(track->sTrack->notes[track->note_index].note, bIdx, volume);
                }
            }
            else
            {
                if (isActive)
                {
                    // Song is over
                    bzrStopNote(bIdx);
                }

                track->usAccum    = 0;
                track->note_index = 0;
                track->sTrack     = NULL;
                // Track is inactive
                return false;
            }
        }
        // Track is active
        return true;
    }
    // Track is inactive
    return false;
}

/**
 * @brief Callback for sound events, both input and output
 * Handle output here, pass input to handleSoundInput()
 *
 * @param sd The sound driver
 * @param in A pointer to read samples from. May be NULL
 * @param out A pointer to write samples to. May be NULL
 * @param samples_R The number of samples to read
 * @param samples_W The number of samples to write
 */
void EmuSoundCb(struct SoundDriver* sd, short* in, short* out, int samples_R, int samples_W)
{
    // Pass to microphone
    handleSoundInput(sd, in, out, samples_R, samples_W);

    // If this is an output callback, and there are samples to write
    if (samples_W && out)
    {
        // Keep track of our place in the wave
        static float placeInWave[NUM_BUZZERS] = {0, 0};

        for (int bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
        {
            // If there is a note to play
            if (buzzers[bIdx].cFreq)
            {
                float transitionPoint = (2 * M_PI * buzzers[bIdx].vol) / 8192;
                // For each sample
                for (int i = 0; i < samples_W; i += 2)
                {
                    // Write the sample, interleaved
                    out[i + bIdx] = 1024 * ((placeInWave[bIdx] < transitionPoint) ? 1 : -1);
                    // Advance the place in the wave
                    placeInWave[bIdx] += ((2 * M_PI * buzzers[bIdx].cFreq) / ((float)SAMPLING_RATE));
                    // Keep it bound between 0 and 2*PI
                    if (placeInWave[bIdx] >= (2 * M_PI))
                    {
                        placeInWave[bIdx] -= (2 * M_PI);
                    }
                }
            }
            else
            {
                // No note to play
                for (int i = 0; i < samples_W; i += 2)
                {
                    // Write the sample, interleaved
                    out[i + bIdx] = 0;
                }
                placeInWave[bIdx] = 0;
            }
        }
    }
}

/**
 * @brief Pause the buzzer but do not reset the song
 */
void bzrPause(void)
{
    if (!bzrPaused)
    {
        bzrPaused = true;
        bzrStop(false);
    }
}

/**
 * @brief Resume the buzzer after being paused
 */
void bzrResume(void)
{
    if (bzrPaused)
    {
        bzrPaused = false;

        // For each buzzer, resume playing the tone before pausing
        for (uint16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
        {
            buzzer_t* bzr = &buzzers[bIdx];
            bzrPlayNote(bzr->cFreq, bIdx, bzr->vol);
        }
    }
}

/**
 * @brief Save the state of the buzzer so that it can be restored later, perhaps
 * after playing a different sound.
 *
 * @return A void-pointer which can be passed back to bzrRestore()
 */
void* bzrSave(void)
{
    bzrPause();

    bzrTrack_t* result = malloc(sizeof(bzrTrack_t) * NUM_BUZZERS * 2);
    for (uint16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
    {
        memcpy(&result[bIdx * 2], &buzzers[bIdx].bgm, sizeof(bzrTrack_t));
        memcpy(&result[bIdx * 2 + 1], &buzzers[bIdx].sfx, sizeof(bzrTrack_t));
    }

    return (void*)result;
}

/**
 * @brief Restore the state of the buzzer from a void-pointer returned by bzrSave()
 *
 * The data passed pointer will be freed by this call.
 *
 * @param data The saved state of the buzzer, returned by bzrSave()
 */
void bzrRestore(void* data)
{
    bzrTrack_t* buzzerState = (bzrTrack_t*)data;
    for (uint16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
    {
        memcpy(&buzzers[bIdx].bgm, &buzzerState[bIdx * 2], sizeof(bzrTrack_t));
        memcpy(&buzzers[bIdx].sfx, &buzzerState[bIdx * 2 + 1], sizeof(bzrTrack_t));
    }

    free(data);
}
