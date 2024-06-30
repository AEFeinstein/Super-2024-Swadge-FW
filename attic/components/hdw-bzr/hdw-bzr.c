//==============================================================================
// Includes
//==============================================================================

#include <string.h>

#include <esp_log.h>
#include <esp_timer.h>
#include <esp_attr.h>
#include <driver/gptimer.h>

#include "hdw-bzr.h"

//==============================================================================
// Defines
//==============================================================================

#define LEDC_MODE LEDC_LOW_SPEED_MODE //!< Low speed mode is sufficient

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A single note and duration to play on the buzzer
 */
typedef struct
{
    noteFrequency_t note; ///< Note frequency, in Hz
    int32_t timeMs;       ///< Note duration, in ms
} musicalNote_t;

/**
 * @brief A list of notes and durations to play on the buzzer
 */
typedef struct
{
    int32_t numNotes;      ///< The number of notes in this song
    int32_t loopStartNote; ///< The note index to restart at, if looping
    musicalNote_t* notes;  ///< An array of notes in the song
} songTrack_t;

/**
 * @brief A collection of lists of notes and durations to play on the buzzers
 */
typedef struct
{
    int16_t numTracks;   ///< The number of tracks in this song
    bool shouldLoop;     ///< true if the song should loop, false if it should play once
    songTrack_t* tracks; ///< The tracks for this song
} song_t;

/**
 * @brief A track for a song on the buzzer. This plays notes from a songTrack_t
 */
typedef struct
{
    int32_t usAccum;           ///< Accumulated time for the current musicalNote_t
    int32_t note_index;        ///< The index of the current musicalNote_t in the song
    const songTrack_t* sTrack; ///< The song being played
    bool should_loop;          ///< Whether or not this track should loop when done
} bzrTrack_t;

/**
 * @brief A physical buzzer that has BGM and SFX tracks
 */
typedef struct
{
    ledc_timer_t ledcTimer;     ///< LEDC timer to play notes
    ledc_channel_t ledcChannel; ///< LEDC channel to play notes
    noteFrequency_t cFreq;      ///< The current frequency of the note being played
    uint16_t volume;            ///< This track's volume, from 0 (off) to 4096 (max)
    bzrTrack_t bgm;             ///< The BGM track for this buzzer
    bzrTrack_t sfx;             ///< The SFX track for this buzzer
} buzzer_t;

/**
 * @brief A struct for holding buzzer save-restore data
 *
 */
typedef struct
{
    bzrTrack_t tracks[NUM_BUZZERS * 2];
    song_t songs[2];
    songTrack_t songTracks[NUM_BUZZERS * 2];
} bzrSaveState_t;

//==============================================================================
// Variables
//==============================================================================

/// @brief Timer to check for note transitions
static gptimer_handle_t bzrTimer = NULL;
/// @brief Track if the buzzer timer is active or not
static bool bzrTimerActive = false;

/// @brief Array of buzzers, left and right
static buzzer_t buzzers[NUM_BUZZERS] = {0};

/// @brief BGM volume
static uint16_t bgmVolume = 0;
/// @brief SFX volume
static uint16_t sfxVolume = 0;

/// @brief Track if the buzzer is paused or not
static bool bzrPaused = false;

/// @brief A flag to set if bgmDoneCb should be called from the main loop
static bool bgmDoneFlag = false;
/// @brief A flag to set if sfxDoneCb should be called from the main loop
static bool sfxDoneFlag = false;
/// @brief A callback function to call after SFX finishes. This is not called if the song is manually stopped or another
/// song starts before this one finishes
static songFinishedCbFn sfxDoneCb = NULL;
/// @brief A callback function to call after BGM finishes. This is not called if the song is manually stopped or another
/// song starts before this one finishes
static songFinishedCbFn bgmDoneCb = NULL;

/// @brief MIDI file reader used for the SFX channel
static song_t sfxSong = {0};

/// @brief MIDI file reader used for the BGM channel
static song_t bgmSong = {0};

/// @brief Boolean to reset clocks after buzzer is restored
static volatile bool bzrIsRestored = false;

// MIDI notes start at C-2, whereas the buzzer starts at C0
// So, just repeat the bottom octave a few times?
// We could just transpose up two octaves instead, so C-2 becomes C0 and C8 becomes C10
// That might be strange
static noteFrequency_t midiNoteMap[] = {
    // C-2 to B-2
    C_0,
    C_SHARP_0,
    D_0,
    D_SHARP_0,
    E_0,
    F_0,
    F_SHARP_0,
    G_0,
    G_SHARP_0,
    A_0,
    A_SHARP_0,
    B_0,

    // C-1 to B-1
    C_0,
    C_SHARP_0,
    D_0,
    D_SHARP_0,
    E_0,
    F_0,
    F_SHARP_0,
    G_0,
    G_SHARP_0,
    A_0,
    A_SHARP_0,
    B_0,

    // Actual C0 to B0
    C_0,
    C_SHARP_0,
    D_0,
    D_SHARP_0,
    E_0,
    F_0,
    F_SHARP_0,
    G_0,
    G_SHARP_0,
    A_0,
    A_SHARP_0,
    B_0,

    // C1
    C_1,
    C_SHARP_1,
    D_1,
    D_SHARP_1,
    E_1,
    F_1,
    F_SHARP_1,
    G_1,
    G_SHARP_1,
    A_1,
    A_SHARP_1,
    B_1,

    // C2
    C_2,
    C_SHARP_2,
    D_2,
    D_SHARP_2,
    E_2,
    F_2,
    F_SHARP_2,
    G_2,
    G_SHARP_2,
    A_2,
    A_SHARP_2,
    B_2,

    // C3
    C_3,
    C_SHARP_3,
    D_3,
    D_SHARP_3,
    E_3,
    F_3,
    F_SHARP_3,
    G_3,
    G_SHARP_3,
    A_3,
    A_SHARP_3,
    B_3,

    // C4
    C_4,
    C_SHARP_4,
    D_4,
    D_SHARP_4,
    E_4,
    F_4,
    F_SHARP_4,
    G_4,
    G_SHARP_4,
    A_4,
    A_SHARP_4,
    B_4,

    // C5
    C_5,
    C_SHARP_5,
    D_5,
    D_SHARP_5,
    E_5,
    F_5,
    F_SHARP_5,
    G_5,
    G_SHARP_5,
    A_5,
    A_SHARP_5,
    B_5,

    // C6
    C_6,
    C_SHARP_6,
    D_6,
    D_SHARP_6,
    E_6,
    F_6,
    F_SHARP_6,
    G_6,
    G_SHARP_6,
    A_6,
    A_SHARP_6,
    B_6,

    // C7
    C_7,
    C_SHARP_7,
    D_7,
    D_SHARP_7,
    E_7,
    F_7,
    F_SHARP_7,
    G_7,
    G_SHARP_7,
    A_7,
    A_SHARP_7,
    B_7,

    // C8
    C_8,
    C_SHARP_8,
    D_8,
    D_SHARP_8,
    E_8,
    F_8,
    F_SHARP_8,
    G_8,

    // End of MIDI note range
};

//==============================================================================
// Functions Prototypes
//==============================================================================

static void initSingleBuzzer(buzzer_t* buzzer, gpio_num_t bzrGpio, ledc_timer_t ledcTimer, ledc_channel_t ledcChannel);
static bool buzzer_check_next_note_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);
static bool buzzer_track_check_next_note(bzrTrack_t* track, buzzerPlayTrack_t bIdx, uint16_t volume, bool isActive,
                                         bool isBgm, int32_t tElapsedUs);
static void bzrPlayTrack(bzrTrack_t* trackL, bzrTrack_t* trackR, const song_t* song, buzzerPlayTrack_t track);

//==============================================================================
// Const variables
//==============================================================================

static const uint16_t volLevels[] = {
    0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the buzzers
 *
 * @param bzrGpioL The GPIO the left buzzer is attached to
 * @param ledcTimerL The LEDC timer used to drive the left buzzer
 * @param ledcChannelL The LEDC channel used to drive the left buzzer
 * @param bzrGpioR The GPIO the right buzzer is attached to
 * @param ledcTimerR The LEDC timer used to drive the right buzzer
 * @param ledcChannelR The LEDC channel used to drive the right buzzer
 * @param _bgmVolume Starting background sound volume, 0 to 4096
 * @param _sfxVolume Starting effects sound volume, 0 to 4096
 */
void initBuzzer(gpio_num_t bzrGpioL, ledc_timer_t ledcTimerL, ledc_channel_t ledcChannelL, gpio_num_t bzrGpioR,
                ledc_timer_t ledcTimerR, ledc_channel_t ledcChannelR, uint16_t _bgmVolume, uint16_t _sfxVolume)
{
    // Set initial volume
    bzrSetBgmVolume(_bgmVolume);
    bzrSetSfxVolume(_sfxVolume);

    // Save the LEDC timers and channels
    initSingleBuzzer(&buzzers[BZR_LEFT], bzrGpioL, ledcTimerL, ledcChannelL);
    initSingleBuzzer(&buzzers[BZR_RIGHT], bzrGpioR, ledcTimerR, ledcChannelR);

    // Initialize the timer
    gptimer_config_t timer_config = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = 1000 * 1000, // 1MHz
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &bzrTimer));

    // Configure the hardware timer to check for note transitions
    gptimer_alarm_config_t config = {
        .alarm_count                = 5000, // Check every 5000 ticks of a 1MHz clock, i.e. every 5ms
        .reload_count               = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(bzrTimer, &config));

    // Configure the ISR
    gptimer_event_callbacks_t callbacks = {
        .on_alarm = buzzer_check_next_note_isr,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(bzrTimer, &callbacks, NULL));

    // Don't start the timer until a song is played
    ESP_ERROR_CHECK(gptimer_enable(bzrTimer));
    gptimer_stop(bzrTimer);
    bzrTimerActive = false;
}

/**
 * @brief Initialize a single buzzer's LEDC
 *
 * @param buzzer The buzzer to initialize
 * @param bzrGpio The GPIO the buzzer is on
 * @param ledcTimer The LEDC timer to use for this buzzer
 * @param ledcChannel The LEDC channel to use for this buzzer
 */
static void initSingleBuzzer(buzzer_t* buzzer, gpio_num_t bzrGpio, ledc_timer_t ledcTimer, ledc_channel_t ledcChannel)
{
    // Set variables in the struct
    buzzer->ledcChannel = ledcChannel;
    buzzer->ledcTimer   = ledcTimer;
    buzzer->cFreq       = SILENCE;
    buzzer->volume      = 0;
    memset(&buzzer->bgm, 0, sizeof(bzrTrack_t));
    memset(&buzzer->sfx, 0, sizeof(bzrTrack_t));

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_MODE,
        .timer_num       = buzzer->ledcTimer,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz         = C_4, // Reasonable place to start
        .clk_cfg         = LEDC_USE_APB_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel    = buzzer->ledcChannel,
        .timer_sel  = buzzer->ledcTimer,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = bzrGpio,
        .duty       = bgmVolume, // Duty cycle is equivalent to volume
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // Stop all buzzer output
    ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, buzzer->ledcChannel, 0));
}

/**
 * @brief Deinitialize the buzzer
 */
void deinitBuzzer(void)
{
    deinitSong(&bgmSong);
    deinitSong(&sfxSong);
    ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, buzzers[BZR_LEFT].ledcChannel, 0));
    ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, buzzers[BZR_RIGHT].ledcChannel, 0));
    ESP_ERROR_CHECK(gptimer_stop(bzrTimer));
    ESP_ERROR_CHECK(gptimer_disable(bzrTimer));
}

/**
 * @brief Set the buzzer's bgm volume. setBgmVolumeSetting() should be called instead if the new volume should be
 * persistent through a reboot.
 *
 * @param vol The background volume, 0 to MAX_VOLUME
 */
void bzrSetBgmVolume(uint16_t vol)
{
    bgmVolume = volLevels[vol];
}

/**
 * @brief Set the buzzer's sfx volume. setSfxVolumeSetting() should be called instead if the new volume should be
 * persistent through a reboot.
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
            trackR->should_loop = song->shouldLoop;
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

    // Un-pause if paused
    bzrResume();
}

/**
 * @brief Start playing a background music on the buzzer. This has lower priority
 * than sound effects
 *
 * @param song The song to play as a sequence of notes
 * @param track The track to play on if the song is mono. This is ignored if the song is stereo
 */
void bzrPlayBgm(const midiFile_t* song, buzzerPlayTrack_t track)
{
    deinitSong(&bgmSong);

    if (setupSongFromMidi(&bgmSong, song))
    {
        // Play this song on the BGM track
        bzrPlayTrack(&buzzers[0].bgm, &buzzers[1].bgm, &bgmSong, track);

        // If the timer is off
        if (false == bzrTimerActive)
        {
            // Start timer, the timer function will start the song
            gptimer_start(bzrTimer);
            bzrTimerActive = true;
        }
    }

    // Called without a callback, so null it out
    bgmDoneCb = NULL;
}

/**
 * @brief Start playing a sound effect on the buzzer. This has higher priority
 * than background music
 *
 * @param song The song to play as a sequence of notes
 * @param track The track to play on if the song is mono. This is ignored if the song is stereo
 */
void bzrPlaySfx(const midiFile_t* song, buzzerPlayTrack_t track)
{
    deinitSong(&sfxSong);

    if (setupSongFromMidi(&bgmSong, song))
    {
        // Play this song on the SFX track
        bzrPlayTrack(&buzzers[0].sfx, &buzzers[1].sfx, &sfxSong, track);

        // If the timer is off
        if (false == bzrTimerActive)
        {
            // Start timer, the timer function will start the song
            gptimer_start(bzrTimer);
            bzrTimerActive = true;
        }
    }

    // Called without a callback, so null it out
    sfxDoneCb = NULL;
}

/**
 * @brief Start playing a background music on the buzzer. This has lower priority
 * than sound effects. cbFn will be called when the music finishes playing
 *
 * @param song The song to play as a sequence of notes
 * @param track The track to play on if the song is mono. This is ignored if the song is stereo
 * @param cbFn
 */
void bzrPlayBgmCb(const song_t* song, buzzerPlayTrack_t track, songFinishedCbFn cbFn)
{
    bzrPlayBgm(song, track);
    bgmDoneCb = cbFn;
}

/**
 * @brief Start playing a sound effect on the buzzer. This has higher priority
 * than background music. cbFn will be called when the effect finishes playing
 *
 * @param song The song to play as a sequence of notes
 * @param track The track to play on if the song is mono. This is ignored if the song is stereo
 * @param cbFn A callback function to call when the effect finishes playing
 */
void bzrPlaySfxCb(const song_t* song, buzzerPlayTrack_t track, songFinishedCbFn cbFn)
{
    bzrPlaySfx(song, track);
    sfxDoneCb = cbFn;
}

/**
 * @brief Stop the buzzer from playing anything
 *
 * @param resetTracks true to reset track data as well
 */
void bzrStop(bool resetTracks)
{
    // Stop the timer to check notes
    gptimer_stop(bzrTimer);
    bzrTimerActive = false;

    // Stop the note
    bzrStopNote(BZR_LEFT);
    bzrStopNote(BZR_RIGHT);

    if (resetTracks)
    {
        // Clear internal variables
        for (uint8_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
        {
            memset(&(buzzers[bIdx].bgm), 0, sizeof(bzrTrack_t));
            memset(&(buzzers[bIdx].sfx), 0, sizeof(bzrTrack_t));
        }
    }
}

/////////////////////////////

/**
 * @brief Start playing a single note on the buzzer.
 * This note will play until stopped.
 * This has IRAM_ATTR because it may be called from an interrupt
 *
 * @param freq The frequency of the note
 * @param track The track to play the note on
 * @param volume The volume, 0 to 4096
 */
void IRAM_ATTR bzrPlayNote(noteFrequency_t freq, buzzerPlayTrack_t track, uint16_t volume)
{
    for (int16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
    {
        if (BZR_STEREO == track || bIdx == track)
        {
            buzzer_t* bzr = &buzzers[bIdx];
            if (SILENCE == freq)
            {
                bzr->cFreq = SILENCE;
                ledc_stop(LEDC_MODE, bzr->ledcChannel, 0);
            }
            else if (bzr->cFreq != freq)
            {
                bzr->cFreq = freq;
                // Set the frequency
                ledc_set_freq(LEDC_MODE, bzr->ledcTimer, bzr->cFreq);
                // Set duty to 50%
                ledc_set_duty(LEDC_MODE, bzr->ledcChannel, volume);
                // Update duty to start the buzzer
                ledc_update_duty(LEDC_MODE, bzr->ledcChannel);
            }
        }
    }
}

/**
 * @brief Stop playing a single note on the buzzer
 * This has IRAM_ATTR because it may be called from an interrupt
 */
void IRAM_ATTR bzrStopNote(buzzerPlayTrack_t track)
{
    for (int16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
    {
        if (BZR_STEREO == track || bIdx == track)
        {
            buzzer_t* bzr = &buzzers[bIdx];
            bzr->cFreq    = SILENCE;
            ledc_stop(LEDC_MODE, bzr->ledcChannel, 0);
        }
    }
}

/////////////////////////////

/**
 * @brief Check if there is a new note to play on the buzzer.
 * This is called periodically in a timer interrupt
 *
 * This has IRAM_ATTR because it is an interrupt
 *
 * @param timer Timer handle created by gptimer_new_timer
 * @param edata Alarm event data, fed by driver
 * @param user_ctx User data, passed from gptimer_register_event_callbacks
 * @return true if a high priority task has been waken up by this function, false otherwise
 */
static bool IRAM_ATTR buzzer_check_next_note_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata,
                                                 void* user_ctx)
{
    // Track time between function calls
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

        // If the buzzer is being restored
        if (bzrIsRestored)
        {
            bzrIsRestored = false;
            // Return for once cycle so that tNowUs is set to a sane value
            return false;
        }

        // Don't do much if paused. Check here so that tElapsedUs stays sane
        if (bzrPaused)
        {
            return false;
        }

        for (uint16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
        {
            buzzer_t* bzr = &buzzers[bIdx];
            // Try playing SFX first
            bool sfxIsActive = buzzer_track_check_next_note(&bzr->sfx, bIdx, sfxVolume, true, false, tElapsedUs);
            // Then play BGM if SFX isn't active
            bool bgmIsActive = buzzer_track_check_next_note(&bzr->bgm, bIdx, bgmVolume, !sfxIsActive, true, tElapsedUs);

            // If nothing is playing, but there is BGM (i.e. SFX finished)
            if ((false == sfxIsActive) && (false == bgmIsActive) && (NULL != bzr->bgm.sTrack))
            {
                // Immediately start playing BGM to get back on track faster
                bzrPlayNote(bzr->bgm.sTrack->notes[bzr->bgm.note_index].note, bIdx, bgmVolume);
            }
        }
    }
    return false;
}

/**
 * Check a specific track for notes to be played and queue them for playing.
 * This will always advance through notes in a song, even if it's not the active
 * track
 * This has IRAM_ATTR because it is called from buzzer_check_next_note_isr()
 *
 * @param track The track to advance notes in
 * @param bIdx The index of the buzzer to play on
 * @param volume The volume to play
 * @param isActive true if this is active and should set a note to be played
 *                 false to just advance notes without playing
 * @param isBgm true if this is BGM, false if this is SFX
 * @param tElapsedUs The elapsed time since this function was last called
 * @return true  if this track is playing a note
 *         false if this track is not playing a note
 */
static bool IRAM_ATTR buzzer_track_check_next_note(bzrTrack_t* track, buzzerPlayTrack_t bIdx, uint16_t volume,
                                                   bool isActive, bool isBgm, int32_t tElapsedUs)
{
    // Check if there is a song and there are still notes
    if ((NULL != track->sTrack) && (track->note_index < track->sTrack->numNotes))
    {
        // Check if it's time to play the next note
        bool shouldAdvance = false;
        if (-1 == track->note_index)
        {
            // Index is negative, so the song is just starting
            track->note_index++;
            track->usAccum = 0;
            shouldAdvance  = true;
        }
        else
        {
            // Accumulate time
            track->usAccum += tElapsedUs;
            // Get the current note length
            int32_t noteTimeUs = (1000 * track->sTrack->notes[track->note_index].timeMs);
            // If the note expired
            if (track->usAccum >= noteTimeUs)
            {
                // Decrement the count
                track->usAccum -= noteTimeUs;
                // Move to the next
                track->note_index++;
                shouldAdvance = true;
            }
        }

        // Check if it's time to play the next note
        if (shouldAdvance)
        {
            // Loop if we should
            if (track->should_loop && (track->note_index == track->sTrack->numNotes))
            {
                track->note_index = track->sTrack->loopStartNote;
            }

            // If there is a note
            if (track->note_index < track->sTrack->numNotes)
            {
                if (isActive)
                {
                    bzrPlayNote(track->sTrack->notes[track->note_index].note, bIdx, volume);
                }
            }
            else
            {
                if (isActive)
                {
                    // Set the song to stop
                    bzrStopNote(bIdx);
                }

                // Clear track data
                track->usAccum    = 0;
                track->note_index = 0;
                track->sTrack     = NULL;

                // Raise a flag here but do not directly call the callback because this
                // is in an interrupt. The callback should be called from the main loop
                if (isBgm)
                {
                    bgmDoneFlag = true;
                }
                else
                {
                    sfxDoneFlag = true;
                }

                // Track isn't active
                return false;
            }
        }
        // Track is active
        return true;
    }
    // Track isn't active
    return false;
}

static bool setupSongFromMidi(song_t* song, const midiFile_t* midi)
{
    midiFileReader_t reader;
    if (!initMidiParser(&reader, midi))
    {
        ESP_LOGE("Buzzer", "Could not allocate MIDI parser to convert song");
        return false;
    }

    midiEvent_t event;
    // note counts for each channel
    uint32_t noteOnCount[16]  = {0};
    uint32_t noteOffCount[16] = {0};
    uint32_t trackLengths[16] = {0};
    bool noteAtZero[16]       = {0};
    uint32_t lengthInTicks    = 0;
    uint32_t tempo            = 500000;

    // First, count the note events
    while (midiNextEvent(&reader, &event))
    {
        if (event.type == MIDI_EVENT)
        {
            // TODO: Should this use channel, or track index? It's usually going to be the same...
            uint8_t ch  = event.midi.status & 0x0F;
            uint8_t cmd = (event.midi.status >> 4) & 0x0F;

            if (cmd == 0x8)
            {
                // Note OFF
                noteOffCount[event.track]++;
            }
            else if (cmd == 0x9)
            {
                // Note ON
                noteOnCount[event.track]++;
                if (event.absTime == 0)
                {
                    noteAtZero[event.track] = true;
                }
            }
        }
        else if (event.type == META_EVENT)
        {
            if (event.meta.type == TEMPO)
            {
                tempo = event.meta.tempo;
            }
            else if (event.meta.type == END_OF_TRACK)
            {
                trackLengths[event.track] = event.absTime;

                // Calculate song length
                if (event.absTime > lengthInTicks)
                {
                    lengthInTicks = event.absTime;
                }
            }
        }
    }

    uint8_t usedChannelCount = 0;
    uint8_t usedChannels[NUM_BUZZERS];

    // Figure out which channels to use
    for (int i = 0; i < 16 && usedChannelCount < NUM_BUZZERS; i++)
    {
        if (noteOnCount[i] > 0 && noteOffCount[i] > 0)
        {
            // Map this channel onto one of the buzzers
            usedChannels[usedChannelCount++] = i;
        }
    }

    if (usedChannelCount == 0)
    {
        ESP_LOGE("Buzzer", "No suitable tracks with notes found in song");
        return false;
    }

    song->numTracks = usedChannelCount;
    song->tracks    = heap_caps_calloc(usedChannelCount, sizeof(songTrack_t), MALLOC_CAP_SPIRAM);

    // The index of the next note to write for a track
    uint32_t nextChNote[NUM_BUZZERS] = {0};
    // The absolute start time of the most recent note, in MIDI ticks, for a track
    uint32_t lastNoteStart[NUM_BUZZERS] = {0};

    for (int i = 0; i < usedChannelCount; i++)
    {
        uint8_t mappedChannel = usedChannels[i];
        // We need a note for every note on, a note for every silence, and one extra silence if the first note isn't at
        // the very beginning of the song
        uint32_t noteCount
            = noteOnCount[mappedChannel] + noteOffCount[mappedChannel] + (noteAtZero[mappedChannel] ? 0 : 1);
        song->tracks[i].notes    = heap_caps_malloc((noteCount) * sizeof(musicalNote_t), MALLOC_CAP_SPIRAM);
        song->tracks[i].numNotes = noteCount;

        // If the first note didn't start at 0, add a silence note
        if (noteCount > 0 && !noteAtZero[mappedChannel])
        {
            song->tracks[i].notes[0].note = SILENCE;
            nextChNote[i]++;
        }
    }

    // Rewind to the beginning of the file
    resetMidiParser(&reader);

// Convert from midi ticks to microseconds
#define MIDI_TICKS_TO_MICROS(midiTicks, tempo, div) (((midiTicks) * (tempo)) / (div))

    while (midiNextEvent(&reader, &event))
    {
        if (event.type == MIDI_EVENT)
        {
            uint8_t ch  = event.midi.status & 0x0F;
            uint8_t cmd = (event.midi.status >> 4) & 0x0F;

            if (cmd == 0x8 || cmd == 0x9)
            {
                for (int i = 0; i < usedChannelCount; i++)
                {
                    if (event.track == usedChannels[i])
                    {
                        uint32_t noteIdx = nextChNote[i];
                        if (noteIdx > 0)
                        {
                            // Set the duration of the previous note
                            song->tracks[i].notes[noteIdx-1].timeMs = (MIDI_TICKS_TO_MICROS(event.absTime - lastNoteStart[i], tempo, midi->timeDivision) + 500) / 1000;
                        }

                        if (cmd == 0x9)
                        {
                            // Note ON
                            song->tracks[i].notes[noteIdx].note = midiNoteMap[event.midi.data[0]];
                        }
                        else
                        {
                            // Note OFF
                            song->tracks[i].notes[noteIdx].note = SILENCE;
                        }

                        lastNoteStart[i] = event.absTime;
                        nextChNote[i]++;

                        break;
                    }
                }
            }
        }
    }

    // Set up the time for the last notes
    for (int i = 0; i < usedChannelCount; i++)
    {
        song->tracks[i].notes[nextChNote[i] - 1].timeMs = (MIDI_TICKS_TO_MICROS(lengthInTicks - lastNoteStart[i], tempo, midi->timeDivision) + 500) / 1000;
        if (nextChNote[i] != song->tracks[i].numNotes)
        {
            ESP_LOGW("Buzzer", "Converting MIDI to song_t - expected to set %" PRId32 " notes but there were %" PRIu32, song->tracks[i].numNotes, nextChNote[i]);
        }
    }

    deinitMidiParser(&reader);

    return true;
}

static void deinitSong(song_t* song)
{
    if (song != NULL)
    {
        if (song->tracks != NULL)
        {
            for (int i = 0; i < song->numTracks; i++)
            {
                free(song->tracks[i].notes);
            }

            free(song->tracks);
            song->tracks = NULL;
        }

        song->numTracks = 0;
    }
}

/**
 * @brief Check if a song has finished playing and call the appropriate callback if applicable
 */
void bzrCheckSongDone(void)
{
    if (true == bgmDoneFlag)
    {
        bgmDoneFlag = false;
        if (NULL != bgmDoneCb)
        {
            bgmDoneCb();
            bgmDoneCb = NULL;
        }
    }

    if (true == sfxDoneFlag)
    {
        sfxDoneFlag = false;
        if (NULL != sfxDoneCb)
        {
            sfxDoneCb();
            sfxDoneCb = NULL;
        }
    }
}

/**
 * @brief Pause the buzzer but do not reset the song
 *
 * @return true if the buzzer was running and paused, false if it was not running to begin with
 */
bool bzrPause(void)
{
    if (!bzrPaused)
    {
        bzrPaused = true;
        bzrPlayNote(SILENCE, BZR_STEREO, 0);
        return true;
    }
    return false;
}

/**
 * @brief Resume the buzzer after being paused
 */
void bzrResume(void)
{
    if (bzrPaused || !bzrTimerActive)
    {
        // Mark it as not paused
        bzrPaused = false;

        // Set this to restore the elapsed clock in the ISR
        bzrIsRestored = true;

        if (!bzrTimerActive)
        {
            // Restart timer stopped by bzrStop()
            gptimer_start(bzrTimer);
            bzrTimerActive = true;
        }

        // For each buzzer, resume playing the tone before pausing
        for (uint16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
        {
            buzzer_t* bzr = &buzzers[bIdx];
            bzrPlayNote(bzr->cFreq, bIdx, bzr->volume);
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

    // Allocate the save state
    bzrSaveState_t* state = malloc(sizeof(bzrSaveState_t));

    // Copy the buzzer state
    for (uint16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
    {
        memcpy(&state->tracks[bIdx * 2], &buzzers[bIdx].bgm, sizeof(bzrTrack_t));
        memcpy(&state->tracks[bIdx * 2 + 1], &buzzers[bIdx].sfx, sizeof(bzrTrack_t));
    }

    // Copy the song struct
    memcpy(&state->songs[0], &bgmSong, sizeof(song_t));
    memcpy(&state->songs[1], &sfxSong, sizeof(song_t));

    // Handle the individually-allocated bits of the song
    if (bgmSong.numTracks > 0 && bgmSong.tracks != NULL)
    {
        state->songs[0].tracks = malloc(bgmSong.numTracks * sizeof(songTrack_t));
        memcpy(state->songs[0].tracks, bgmSong.tracks, bgmSong.numTracks * sizeof(songTrack_t));

        for (int i = 0; i < bgmSong.numTracks; i++)
        {
            state->songs[0].tracks[i].notes = heap_caps_malloc(bgmSong.tracks[i].numNotes * sizeof(musicalNote_t), MALLOC_CAP_SPIRAM);
            memcpy(state->songs[0].tracks[i].notes, bgmSong.tracks[i].notes, bgmSong.tracks[i].numNotes * sizeof(musicalNote_t));

            // Update the reference to the track from the song state if necessary
            if (buzzers[0].bgm.sTrack == &bgmSong.tracks[i])
            {
                state->tracks[0].sTrack = &state->songTracks[i];
            }

            if (buzzers[1].bgm.sTrack == &bgmSong.tracks[i])
            {
                state->tracks[1].sTrack = &state->songTracks[i];
            }
        }
    }
    else
    {
        state->songs[0].tracks = NULL;
    }

    if (sfxSong.numTracks > 0 && sfxSong.tracks != NULL)
    {
        state->songs[1].tracks = malloc(sfxSong.numTracks * sizeof(songTrack_t));
        memcpy(state->songs[1].tracks, sfxSong.tracks, sfxSong.numTracks * sizeof(songTrack_t));

        for (int i = 0; i < sfxSong.numTracks; i++)
        {
            state->songs[1].tracks[i].notes = heap_caps_malloc(sfxSong.tracks[i].numNotes * sizeof(musicalNote_t), MALLOC_CAP_SPIRAM);
            memcpy(state->songs[1].tracks[i].notes, sfxSong.tracks[i].notes, sfxSong.tracks[i].numNotes * sizeof(musicalNote_t));

            // Update the reference to the track from the song state if necessary
            if (buzzers[0].sfx.sTrack == &sfxSong.tracks[i])
            {
                state->tracks[0].sTrack = &state->songTracks[2 + i];
            }

            if (buzzers[1].sfx.sTrack == &bgmSong.tracks[i])
            {
                state->tracks[1].sTrack = &state->songTracks[2 + i];
            }
        }
    }
    else
    {
        state->songs[1].tracks = NULL;
    }

    return (void*)state;
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
    bzrSaveState_t* state = (bzrSaveState_t*)data;
    for (uint16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
    {
        memcpy(&buzzers[bIdx].bgm, &state->tracks[bIdx * 2], sizeof(bzrTrack_t));
        memcpy(&buzzers[bIdx].sfx, &state->tracks[bIdx * 2 + 1], sizeof(bzrTrack_t));
    }

    memcpy(&bgmSong, &state->songs[0], sizeof(song_t));
    memcpy(&sfxSong, &state->songs[1], sizeof(song_t));

    // No need to free the other data, as it's now the song
    free(data);

    bzrResume();
}

/**
 * @brief Get the actual volume level from the setting
 *
 * @param setting The volume level setting
 * @return The actual volume
 */
uint16_t volLevelFromSetting(uint16_t setting)
{
    if (setting < (sizeof(volLevels) / sizeof(volLevels[0])))
    {
        return volLevels[setting];
    }
    return 0;
}
