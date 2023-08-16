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

typedef struct
{
    const song_t* song;
    uint32_t note_index;
    int64_t start_time;
    uint16_t volume;
} emu_buzzer_t;

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

// Output buzzer
static uint16_t buzzerNote    = SILENCE;
static uint16_t buzzerVol     = 4096;
static emu_buzzer_t emuBzrBgm = {0};
static emu_buzzer_t emuBzrSfx = {0};

//==============================================================================
// Function Prototypes
//==============================================================================

static bool buzzer_track_check_next_note(emu_buzzer_t* track, bool isActive);
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
 * @param ledcChannelL THe LEDC channel used to drive the left buzzer
 * @param bzrGpioR The GPIO the right buzzer is attached to
 * @param ledcTimerR The LEDC timer used to drive the right buzzer
 * @param ledcChannelR THe LEDC channel used to drive the right buzzer
 * @param bgmVolume Starting background sound volume, 0 to 4096
 * @param sfxVolume Starting effects sound volume, 0 to 4096
 */
void initBuzzer(gpio_num_t bzrGpioL, ledc_timer_t ledcTimerL, ledc_channel_t ledcChannelL, gpio_num_t bzrGpioR,
                ledc_timer_t ledcTimerR, ledc_channel_t ledcChannelR, uint16_t bgmVolume, uint16_t sfxVolume)
{
    emuBzrBgm.volume = bgmVolume;
    emuBzrSfx.volume = sfxVolume;

    bzrStop();
    if (!soundDriver)
    {
        soundDriver = InitSound(0, EmuSoundCb, SAMPLING_RATE, 1, 1, 256, 0, 0);
    }
    memset(&emuBzrBgm, 0, sizeof(emuBzrBgm));
    memset(&emuBzrSfx, 0, sizeof(emuBzrSfx));

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
#if defined(_WIN32)
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
 * @param vol The background volume, 0 to 13
 */
void bzrSetBgmVolume(uint16_t vol)
{
    emuBzrBgm.volume = volLevels[vol];
}

/**
 * @brief Set the buzzer's sfx volume
 *
 * @param vol The background volume, 0 to 13
 */
void bzrSetSfxVolume(uint16_t vol)
{
    emuBzrSfx.volume = volLevels[vol];
}

/**
 * @brief Start playing a background music on the buzzer. This has lower priority
 * than sound effects
 *
 * @param song The song to play as a sequence of notes
 * @param channel TODO
 */
void bzrPlayBgm(const song_t* song, buzzerPlayChannel_t channel)
{
    if (0 == emuBzrBgm.volume)
    {
        return;
    }

    // Save the song pointer
    emuBzrBgm.song       = song;
    emuBzrBgm.note_index = 0;
    emuBzrBgm.start_time = esp_timer_get_time();

    if (NULL == emuBzrSfx.song)
    {
        // Start playing the first note
        bzrPlayNote(emuBzrBgm.song->notes[0].note, BZR_STEREO, emuBzrBgm.volume); // TODO stereo
    }
}

/**
 * @brief Start playing a sound effect on the buzzer. This has higher priority
 * than background music
 *
 * @param song The song to play as a sequence of notes
 * @param channel TODO
 */
void bzrPlaySfx(const song_t* song, buzzerPlayChannel_t channel)
{
    if (0 == emuBzrSfx.volume)
    {
        return;
    }

    // Save the song pointer
    emuBzrSfx.song       = song;
    emuBzrSfx.note_index = 0;
    emuBzrSfx.start_time = esp_timer_get_time();

    // Start playing the first note
    bzrPlayNote(emuBzrSfx.song->notes[0].note, BZR_STEREO, emuBzrSfx.volume); // TODO stereo
}

/**
 * @brief Stop the buzzer from playing anything
 */
void bzrStop(void)
{
    if ((0 == emuBzrBgm.volume) && (0 == emuBzrSfx.volume))
    {
        return;
    }

    emuBzrBgm.song       = NULL;
    emuBzrBgm.note_index = 0;
    emuBzrBgm.start_time = 0;

    emuBzrSfx.song       = NULL;
    emuBzrSfx.note_index = 0;
    emuBzrSfx.start_time = 0;

    buzzerNote = SILENCE;
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
 * @param channel TODO
 * @param volume The volume, 0 to 4096
 */
void bzrPlayNote(noteFrequency_t freq, buzzerPlayChannel_t channel, uint16_t volume)
{
    buzzerNote = freq;
    buzzerVol  = volume;
}

/**
 * @brief Stop playing a single note on the buzzer
 * This has IRAM_ATTR because it may be called from an interrupt
 *
 * @param channel TODO
 */
void bzrStopNote(buzzerPlayChannel_t channel)
{
    bzrPlayNote(SILENCE, channel, 0);
}

////////////////////////////////

/**
 * @brief Call this periodically to check if the next note in the song should be played
 */
void buzzer_check_next_note(void* arg)
{
    if ((0 == emuBzrBgm.volume) && (0 == emuBzrSfx.volume))
    {
        return;
    }

    bool sfxIsActive = buzzer_track_check_next_note(&emuBzrSfx, true);
    bool bgmIsActive = buzzer_track_check_next_note(&emuBzrBgm, !sfxIsActive);

    // If nothing is playing, but there is BGM (i.e. SFX finished)
    if ((false == sfxIsActive) && (false == bgmIsActive) && (NULL != emuBzrBgm.song))
    {
        // Immediately start playing BGM to get back on track faster
        bzrPlayNote(emuBzrBgm.song->notes[emuBzrBgm.note_index].note,
                    emuBzrBgm.song->notes[emuBzrBgm.note_index].channel, emuBzrBgm.volume); // TODO stereo
    }
}

/**
 * @brief Advance the notes in a specific track and play them if the track is active
 *
 * @param track The track to check notes in
 * @param isActive true to play notes, false to just advance them
 * @return true  if this track is playing a note
 *         false if it is not
 */
static bool buzzer_track_check_next_note(emu_buzzer_t* track, bool isActive)
{
    // Check if there is a song and there are still notes
    if ((NULL != track->song) && (track->note_index < track->song->numNotes))
    {
        // Get the current time
        int64_t cTime = esp_timer_get_time();

        // Check if it's time to play the next note
        if (cTime - track->start_time >= (1000 * track->song->notes[track->note_index].timeMs))
        {
            // Move to the next note
            track->note_index++;
            track->start_time = cTime;

            // Loop if requested
            if (track->song->shouldLoop && (track->note_index == track->song->numNotes))
            {
                track->note_index = 0;
            }

            // If there is a note
            if (track->note_index < track->song->numNotes)
            {
                if (isActive)
                {
                    // Play the note
                    bzrPlayNote(track->song->notes[track->note_index].note,
                                track->song->notes[track->note_index].channel, track->volume); // TODO stereo
                }
            }
            else
            {
                if (isActive)
                {
                    // Song is over
                    buzzerNote = SILENCE;
                }

                track->start_time = 0;
                track->note_index = 0;
                track->song       = NULL;
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
        static float placeInWave = 0;

        // If there is a note to play
        if (buzzerNote)
        {
            float transitionPoint = (2 * M_PI * buzzerVol) / 8192;
            // For each sample
            for (int i = 0; i < samples_W; i++)
            {
                // Write the sample
                out[i] = 1024 * ((placeInWave < transitionPoint) ? 1 : -1);
                // Advance the place in the wave
                placeInWave += ((2 * M_PI * buzzerNote) / ((float)SAMPLING_RATE));
                // Keep it bound between 0 and 2*PI
                if (placeInWave >= (2 * M_PI))
                {
                    placeInWave -= (2 * M_PI);
                }
            }
        }
        else
        {
            // No note to play
            memset(out, 0, samples_W * 2);
            placeInWave = 0;
        }
    }
}
