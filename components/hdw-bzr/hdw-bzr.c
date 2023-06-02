//==============================================================================
// Includes
//==============================================================================

#include <esp_log.h>
#include <esp_timer.h>
#include <esp_attr.h>
#include <driver/gptimer.h>

#include "hdw-bzr.h"

//==============================================================================
// Defines
//==============================================================================

// For LEDC
#define LEDC_MODE LEDC_LOW_SPEED_MODE //!< Low speed mode is sufficient

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief A track for a song on the buzzer. This plays notes from a song_t
 */
typedef struct
{
    const song_t* song;  ///< The song_t being played
    uint32_t note_index; ///< The index of the current musicalNote_t in the song
    int64_t start_time;  ///< The time the current musicalNote_t started in the song
    uint16_t volume;     ///< This track's volume, from 0 (off) to 4096 (max)
} buzzerTrack_t;

//==============================================================================
// Variables
//==============================================================================

/// @brief The background track
static buzzerTrack_t bgm;
/// @brief The sound effect track
static buzzerTrack_t sfx;

/// @brief The current frequency of the note being played
static uint32_t cFreq;

/// @brief Timer to check for note transitions
static gptimer_handle_t bzrTimer;

/// @brief LEDC timer to play notes
static ledc_timer_t ledcTimer;
/// @brief LEDC channel to play notes
static ledc_channel_t ledcChannel;

//==============================================================================
// Functions Prototypes
//==============================================================================

static bool buzzer_check_next_note_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata, void* user_ctx);
static bool buzzer_track_check_next_note(buzzerTrack_t* track, bool isActive, int64_t cTime);

//==============================================================================
// Const variables
//==============================================================================

const uint16_t volLevels[] = {
    0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the buzzer
 *
 * @param bzrGpio The GPIO the buzzer is attached to
 * @param _ledcTimer The LEDC timer used to drive the buzzer
 * @param _ledcChannel THe LEDC channel used to drive the buzzer
 * @param _bgmVolume Starting background sound volume, 0 to 4096
 * @param _sfxVolume Starting effects sound volume, 0 to 4096
 */
void initBuzzer(gpio_num_t bzrGpio, ledc_timer_t _ledcTimer, ledc_channel_t _ledcChannel, uint16_t _bgmVolume,
                uint16_t _sfxVolume)
{
    bgm.volume = _bgmVolume;
    sfx.volume = _sfxVolume;

    // Save the LEDC timer and channel
    ledcTimer   = _ledcTimer;
    ledcChannel = _ledcChannel;

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode      = LEDC_MODE,
        .timer_num       = ledcTimer,
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz         = C_4, // Gotta start somewhere, might as well be middle C
        .clk_cfg         = LEDC_USE_APB_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_MODE,
        .channel    = ledcChannel,
        .timer_sel  = ledcTimer,
        .intr_type  = LEDC_INTR_DISABLE,
        .gpio_num   = bzrGpio,
        .duty       = bgm.volume, // Duty cycle is equivalent to volume
        .hpoint     = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // Stop all buzzer output
    ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, ledcChannel, 0));

    ////////////////////////////////////////////////////////////////////////////

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
}

/**
 * @brief Deinitialize the buzzer
 */
void deinitBuzzer(void)
{
    ESP_ERROR_CHECK(ledc_stop(LEDC_MODE, ledcChannel, 0));
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
    bgm.volume = volLevels[vol];
}

/**
 * @brief Set the buzzer's sfx volume. setSfxVolumeSetting() should be called instead if the new volume should be
 * persistent through a reboot.
 *
 * @param vol The background volume, 0 to 13
 */
void bzrSetSfxVolume(uint16_t vol)
{
    sfx.volume = volLevels[vol];
}

/**
 * @brief Start playing a background music on the buzzer. This has lower priority
 * than sound effects
 *
 * @param song The song to play as a sequence of notes
 */
void bzrPlayBgm(const song_t* song)
{
    // Don't play if muted
    if (0 == bgm.volume)
    {
        return;
    }

    bgm.song       = song;
    bgm.note_index = 0;
    bgm.start_time = esp_timer_get_time();

    // If there is no current SFX
    if (NULL == sfx.song)
    {
        // Start playing BGM
        bzrPlayNote(bgm.song->notes[0].note, bgm.volume);
        gptimer_start(bzrTimer);
    }
}

/**
 * @brief Start playing a sound effect on the buzzer. This has higher priority
 * than background music
 *
 * @param song The song to play as a sequence of notes
 */
void bzrPlaySfx(const song_t* song)
{
    // Don't play if muted
    if (0 == sfx.volume)
    {
        return;
    }

    sfx.song       = song;
    sfx.note_index = 0;
    sfx.start_time = esp_timer_get_time();

    // Always play SFX
    bzrPlayNote(sfx.song->notes[0].note, sfx.volume);
    gptimer_start(bzrTimer);
}

/**
 * @brief Stop the buzzer from playing anything
 */
void bzrStop(void)
{
    // Stop the timer to check notes
    gptimer_stop(bzrTimer);

    // Stop the note
    bzrStopNote();

    // Clear internal variables
    bgm.note_index = 0;
    bgm.song       = NULL;
    bgm.start_time = 0;

    sfx.note_index = 0;
    sfx.song       = NULL;
    sfx.start_time = 0;
}

/////////////////////////////

/**
 * @brief Start playing a single note on the buzzer.
 * This note will play until stopped.
 * This has IRAM_ATTR because it may be called from an interrupt
 *
 * @param freq The frequency of the note
 * @param volume The volume, 0 to 4096
 */
void IRAM_ATTR bzrPlayNote(noteFrequency_t freq, uint16_t volume)
{
    if (SILENCE == freq)
    {
        bzrStopNote();
        return;
    }
    else
    {
        if (cFreq != freq)
        {
            cFreq = freq;
            // Set the frequency
            ledc_set_freq(LEDC_MODE, ledcTimer, cFreq);
            // Set duty to 50%
            ledc_set_duty(LEDC_MODE, ledcChannel, volume);
            // Update duty to start the buzzer
            ledc_update_duty(LEDC_MODE, ledcChannel);
        }
    }
}

/**
 * @brief Stop playing a single note on the buzzer
 * This has IRAM_ATTR because it may be called from an interrupt
 */
void IRAM_ATTR bzrStopNote(void)
{
    cFreq = 0;
    ledc_stop(LEDC_MODE, ledcChannel, 0);
}

/////////////////////////////

/**
 * @brief Check if there is a new note to play on the buzzer.
 * This is called periodically in a timer interrupt
 *
 * This has IRAM_ATTR because it is an interrupt
 *
 * @param timer
 * @param edata
 * @param user_ctx
 * @return true
 * @return false
 */
static bool IRAM_ATTR buzzer_check_next_note_isr(gptimer_handle_t timer, const gptimer_alarm_event_data_t* edata,
                                                 void* user_ctx)
{
    // Don't do much if muted
    if ((0 == bgm.volume) && (0 == sfx.volume))
    {
        return false;
    }

    // Get the current time
    int64_t cTime = esp_timer_get_time();

    // Try playing SFX first
    bool sfxIsActive = buzzer_track_check_next_note(&sfx, true, cTime);
    // Then play BGM if SFX isn't active
    bool bgmIsActive = buzzer_track_check_next_note(&bgm, !sfxIsActive, cTime);

    // If nothing is playing, but there is BGM (i.e. SFX finished)
    if ((false == sfxIsActive) && (false == bgmIsActive) && (NULL != bgm.song))
    {
        // Immediately start playing BGM to get back on track faster
        bzrPlayNote(bgm.song->notes[bgm.note_index].note, bgm.volume);
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
 * @param isActive true if this is active and should set a note to be played
 *                 false to just advance notes without playing
 * @return true  if this track is playing a note
 *         false if this track is not playing a note
 */
static bool IRAM_ATTR buzzer_track_check_next_note(buzzerTrack_t* track, bool isActive, int64_t cTime)
{
    // Check if there is a song and there are still notes
    if ((NULL != track->song) && (track->note_index < track->song->numNotes))
    {
        // Check if it's time to play the next note
        if (cTime - track->start_time >= (1000 * track->song->notes[track->note_index].timeMs))
        {
            // Move to the next note
            track->note_index++;
            track->start_time = cTime;

            // Loop if we should
            if (track->song->shouldLoop && (track->note_index == track->song->numNotes))
            {
                track->note_index = track->song->loopStartNote;
            }

            // If there is a note
            if (track->note_index < track->song->numNotes)
            {
                if (isActive)
                {
                    // Set the note to be played
                    bzrPlayNote(track->song->notes[track->note_index].note, track->volume);
                }
            }
            else
            {
                if (isActive)
                {
                    // Set the song to stop
                    bzrStopNote();
                }

                // Clear track data
                track->start_time = 0;
                track->note_index = 0;
                track->song       = NULL;
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
