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
 * @brief Types of loops for a track
 */
typedef enum
{
    NO_LOOP,
    MONO_LOOP,
    STEREO_LOOP
} loopType_t;

//==============================================================================
// Structs
//==============================================================================

/**
 * @brief A track for a song on the buzzer. This plays notes from a songTrack_t
 */
typedef struct
{
    int64_t start_time;        ///< The time the current musicalNote_t started in the song
    int32_t note_index;        ///< The index of the current musicalNote_t in the song
    const songTrack_t* sTrack; ///< The song being played
    loopType_t should_loop;    ///< Whether or not this track should loop when done
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

//==============================================================================
// Functions Prototypes
//==============================================================================

static void initSingleBuzzer(buzzer_t* buzzer, gpio_num_t bzrGpio, ledc_timer_t ledcTimer, ledc_channel_t ledcChannel);
static bool buzzer_check_next_note_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);
static bool buzzer_track_check_next_note(bzrTrack_t* track, buzzerPlayTrack_t bIdx, uint16_t volume, bool isActive,
                                         int64_t cTime, bool* looped);
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
    bgmVolume = _bgmVolume;
    sfxVolume = _sfxVolume;

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
    ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, buzzers[BZR_LEFT].ledcChannel, 0));
    ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, buzzers[BZR_RIGHT].ledcChannel, 0));
    ESP_ERROR_CHECK(gptimer_stop(bzrTimer));
    ESP_ERROR_CHECK(gptimer_disable(bzrTimer));
}

/**
 * @brief Set the buzzer's bgm volume. setBgmVolumeSetting() should be called instead if the new volume should be
 * persistent through a reboot.
 *
 * @param vol The background volume, 0 to 13
 */
void bzrSetBgmVolume(uint16_t vol)
{
    bgmVolume = volLevels[vol];
}

/**
 * @brief Set the buzzer's sfx volume. setSfxVolumeSetting() should be called instead if the new volume should be
 * persistent through a reboot.
 *
 * @param vol The background volume, 0 to 13
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
    int64_t startTime = esp_timer_get_time();
    if (1 == song->numTracks)
    {
        // Mono song, play it on the requested tracks
        if (BZR_STEREO == track || BZR_LEFT == track)
        {
            trackL->sTrack      = &song->tracks[0];
            trackL->note_index  = -1;
            trackL->start_time  = startTime;
            trackL->should_loop = song->shouldLoop ? ((BZR_STEREO == track) ? (STEREO_LOOP) : (MONO_LOOP)) : (NO_LOOP);
        }

        if (BZR_STEREO == track || BZR_RIGHT == track)
        {
            trackR->sTrack      = &song->tracks[0];
            trackR->note_index  = -1;
            trackR->start_time  = startTime;
            trackR->should_loop = song->shouldLoop ? ((BZR_STEREO == track) ? (STEREO_LOOP) : (MONO_LOOP)) : (NO_LOOP);
        }
    }
    else
    {
        // Stereo song, play it on both tracks
        trackL->sTrack      = &song->tracks[0];
        trackL->note_index  = -1;
        trackL->start_time  = startTime;
        trackL->should_loop = song->shouldLoop ? STEREO_LOOP : NO_LOOP;

        trackR->sTrack      = &song->tracks[1];
        trackR->note_index  = -1;
        trackR->start_time  = startTime;
        trackR->should_loop = song->shouldLoop ? STEREO_LOOP : NO_LOOP;
    }
}

/**
 * @brief Start playing a background music on the buzzer. This has lower priority
 * than sound effects
 *
 * @param song The song to play as a sequence of notes
 * @param track The track to play on if the song is mono. This is ignored if the song is stereo
 */
void bzrPlayBgm(const song_t* song, buzzerPlayTrack_t track)
{
    // Don't play if muted
    if (0 == bgmVolume)
    {
        return;
    }

    // Play this song on the BGM track
    bzrPlayTrack(&buzzers[0].bgm, &buzzers[1].bgm, song, track);

    // If the timer is off
    if (false == bzrTimerActive)
    {
        // Start timer, the timer function will start the song
        gptimer_start(bzrTimer);
        bzrTimerActive = true;
    }
}

/**
 * @brief Start playing a sound effect on the buzzer. This has higher priority
 * than background music
 *
 * @param song The song to play as a sequence of notes
 * @param track The track to play on if the song is mono. This is ignored if the song is stereo
 */
void bzrPlaySfx(const song_t* song, buzzerPlayTrack_t track)
{
    // Don't play if muted
    if (0 == sfxVolume)
    {
        return;
    }

    // Play this song on the SFX track
    bzrPlayTrack(&buzzers[0].sfx, &buzzers[1].sfx, song, track);

    // If the timer is off
    if (false == bzrTimerActive)
    {
        // Start timer, the timer function will start the song
        gptimer_start(bzrTimer);
        bzrTimerActive = true;
    }
}

/**
 * @brief Stop the buzzer from playing anything
 */
void bzrStop(void)
{
    // Stop the timer to check notes
    gptimer_stop(bzrTimer);
    bzrTimerActive = false;

    // Stop the note
    bzrStopNote(BZR_LEFT);
    bzrStopNote(BZR_RIGHT);

    // Clear internal variables
    for (uint8_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
    {
        memset(&(buzzers[bIdx].bgm), 0, sizeof(bzrTrack_t));
        memset(&(buzzers[bIdx].sfx), 0, sizeof(bzrTrack_t));
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
    switch (track)
    {
        case BZR_STEREO:
        {
            // Mono means play on both buzzers
            bzrPlayNote(freq, BZR_LEFT, volume);
            bzrPlayNote(freq, BZR_RIGHT, volume);
            break;
        }
        case BZR_LEFT:
        case BZR_RIGHT:
        {
            if (SILENCE == freq)
            {
                bzrStopNote(track);
                return;
            }
            else
            {
                if (buzzers[track].cFreq != freq)
                {
                    buzzers[track].cFreq = freq;
                    // Set the frequency
                    ledc_set_freq(LEDC_MODE, buzzers[track].ledcTimer, buzzers[track].cFreq);
                    // Set duty to 50%
                    ledc_set_duty(LEDC_MODE, buzzers[track].ledcChannel, volume);
                    // Update duty to start the buzzer
                    ledc_update_duty(LEDC_MODE, buzzers[track].ledcChannel);
                }
            }
            break;
        }
    }
}

/**
 * @brief Stop playing a single note on the buzzer
 * This has IRAM_ATTR because it may be called from an interrupt
 */
void IRAM_ATTR bzrStopNote(buzzerPlayTrack_t track)
{
    switch (track)
    {
        case BZR_STEREO:
        {
            // Mono means play on both buzzers
            bzrStopNote(BZR_LEFT);
            bzrStopNote(BZR_RIGHT);
            break;
        }
        case BZR_LEFT:
        case BZR_RIGHT:
        {
            buzzers[track].cFreq = SILENCE;
            ledc_stop(LEDC_MODE, buzzers[track].ledcChannel, 0);
            break;
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
    // Don't do much if muted
    if ((0 == bgmVolume) && (0 == sfxVolume))
    {
        return false;
    }

    // Keep track if a track looped so that the other may loop in sync
    bool sfxLooped[NUM_BUZZERS] = {false, false};
    bool bgmLooped[NUM_BUZZERS] = {false, false};

    // Get the current time
    int64_t cTime = esp_timer_get_time();

    for (uint16_t bIdx = 0; bIdx < NUM_BUZZERS; bIdx++)
    {
        buzzer_t* bzr = &buzzers[bIdx];
        // Try playing SFX first
        bool sfxIsActive = buzzer_track_check_next_note(&bzr->sfx, bIdx, sfxVolume, true, cTime, &sfxLooped[bIdx]);
        // Then play BGM if SFX isn't active
        bool bgmIsActive
            = buzzer_track_check_next_note(&bzr->bgm, bIdx, sfxVolume, !sfxIsActive, cTime, &bgmLooped[bIdx]);

        // If nothing is playing, but there is BGM (i.e. SFX finished)
        if ((false == sfxIsActive) && (false == bgmIsActive) && (NULL != bzr->bgm.sTrack))
        {
            // Immediately start playing BGM to get back on track faster
            bzrPlayNote(bzr->bgm.sTrack->notes[bzr->bgm.note_index].note, bIdx, bgmVolume);
        }
    }

    // Stereo loop SFX in sync
    if (sfxLooped[0])
    {
        buzzers[1].sfx.note_index = buzzers[1].sfx.sTrack->loopStartNote;
        buzzers[1].sfx.start_time = buzzers[0].sfx.start_time;
    }
    else if (sfxLooped[1])
    {
        buzzers[0].sfx.note_index = buzzers[0].sfx.sTrack->loopStartNote;
        buzzers[0].sfx.start_time = buzzers[1].sfx.start_time;
    }

    // Stereo loop BGM in sync
    if (bgmLooped[0])
    {
        buzzers[1].bgm.note_index = buzzers[1].bgm.sTrack->loopStartNote;
        buzzers[1].bgm.start_time = buzzers[0].bgm.start_time;
    }
    else if (bgmLooped[1])
    {
        buzzers[0].bgm.note_index = buzzers[0].bgm.sTrack->loopStartNote;
        buzzers[0].bgm.start_time = buzzers[1].bgm.start_time;
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
 * @param cTime The current system time in microseconds
 * @param looped Output parameter, true if this looped and the other track should loop too
 * @return true  if this track is playing a note
 *         false if this track is not playing a note
 */
static bool IRAM_ATTR buzzer_track_check_next_note(bzrTrack_t* track, buzzerPlayTrack_t bIdx, uint16_t volume,
                                                   bool isActive, int64_t cTime, bool* looped)
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
            track->start_time = cTime;
            shouldAdvance     = true;
        }
        else
        {
            // Get the current note length
            int32_t noteTimeUs = (1000 * track->sTrack->notes[track->note_index].timeMs);
            // If the note expired
            if (cTime >= track->start_time + noteTimeUs)
            {
                // Move to the next, accumulating start_time for accuracy
                track->note_index++;
                track->start_time += noteTimeUs;
                shouldAdvance = true;
            }
        }

        // Check if it's time to play the next note
        if (shouldAdvance)
        {
            // Loop if we should
            if (track->should_loop && (track->note_index == track->sTrack->numNotes))
            {
                if (STEREO_LOOP == track->should_loop)
                {
                    *looped = true;
                }
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
                track->start_time = 0;
                track->note_index = 0;
                track->sTrack     = NULL;
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
