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
} emu_buzzer_t;

//==============================================================================
// Variables
//==============================================================================

/// The sound driver
static struct SoundDriver* sounddriver = NULL;

// Output buzzer
static uint16_t buzzernote    = SILENCE;
static emu_buzzer_t emuBzrBgm = {0};
static emu_buzzer_t emuBzrSfx = {0};

// Keep track of muted state
static bool emuBgmMuted;
static bool emuSfxMuted;

//==============================================================================
// Function Prototypes
//==============================================================================

static bool buzzer_track_check_next_note(emu_buzzer_t* track, bool isActive);
void buzzer_check_next_note(void* arg);
void EmuSoundCb(struct SoundDriver* sd, short* in, short* out, int samplesr, int samplesp);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the buzzer
 *
 * @param bzrGpio The GPIO the buzzer is attached to
 * @param _ledcTimer The LEDC timer used to drive the buzzer
 * @param _ledcChannel THe LEDC channel used to drive the buzzer
 * @param _isBgmMuted True if background music is muted, false otherwise
 * @param _isSfxMuted True if sound effects are muted, false otherwise
 */
void initBuzzer(gpio_num_t bzrGpio, ledc_timer_t _ledcTimer, ledc_channel_t _ledcChannel, bool _isBgmMuted,
                bool _isSfxMuted)
{
    emuBgmMuted = _isBgmMuted;
    emuSfxMuted = _isBgmMuted;

    bzrStop();
    if (!sounddriver)
    {
        sounddriver = InitSound(0, EmuSoundCb, SAMPLING_RATE, 1, 1, 256, 0, 0);
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
    if (sounddriver)
    {
#if defined(_WIN32)
        CloseSound(NULL);
#else
        CloseSound(sounddriver); // when calling this on Windows, it halts
#endif
        sounddriver = NULL;
    }
}

/**
 * @brief Set the buzzer's bgm mute status
 *
 * @param _isBgmMuted True if background music is muted, false otherwise
 */
void bzrSetBgmIsMuted(bool _isBgmMuted)
{
    emuBgmMuted = _isBgmMuted;
}

/**
 * @brief Set the buzzer's sfx mute status
 *
 * @param _isSfxMuted True if sound effects are muted, false otherwise
 */
void bzrSetSfxIsMuted(bool _isSfxMuted)
{
    emuSfxMuted = _isSfxMuted;
}

/**
 * @brief Start playing a background music on the buzzer. This has lower priority
 * than sound effects
 *
 * @param song The song to play as a sequence of notes
 */
void bzrPlayBgm(const song_t* song)
{
    if (emuBgmMuted)
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
        bzrPlayNote(emuBzrBgm.song->notes[0].note);
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
    if (emuSfxMuted)
    {
        return;
    }

    // Save the song pointer
    emuBzrSfx.song       = song;
    emuBzrSfx.note_index = 0;
    emuBzrSfx.start_time = esp_timer_get_time();

    // Start playing the first note
    bzrPlayNote(emuBzrSfx.song->notes[0].note);
}

/**
 * @brief Stop the buzzer from playing anything
 */
void bzrStop(void)
{
    if (emuBgmMuted && emuSfxMuted)
    {
        return;
    }

    emuBzrBgm.song       = NULL;
    emuBzrBgm.note_index = 0;
    emuBzrBgm.start_time = 0;

    emuBzrSfx.song       = NULL;
    emuBzrSfx.note_index = 0;
    emuBzrSfx.start_time = 0;

    buzzernote = SILENCE;
    bzrPlayNote(SILENCE);
}

/////////////////////////////

/**
 * @brief Start playing a single note on the buzzer.
 * This note will play until stopped.
 * This has IRAM_ATTR because it may be called from an interrupt
 *
 * @param freq The frequency of the note
 */
void bzrPlayNote(noteFrequency_t freq)
{
    buzzernote = freq;
}

/**
 * @brief Stop playing a single note on the buzzer
 * This has IRAM_ATTR because it may be called from an interrupt
 */
void bzrStopNote(void)
{
    bzrPlayNote(SILENCE);
}

////////////////////////////////

/**
 * @brief Call this periodically to check if the next note in the song should be played
 */
void buzzer_check_next_note(void* arg)
{
    if (emuBgmMuted && emuSfxMuted)
    {
        return;
    }

    bool sfxIsActive = buzzer_track_check_next_note(&emuBzrSfx, true);
    bool bgmIsActive = buzzer_track_check_next_note(&emuBzrBgm, !sfxIsActive);

    // If nothing is playing, but there is BGM (i.e. SFX finished)
    if ((false == sfxIsActive) && (false == bgmIsActive) && (NULL != emuBzrBgm.song))
    {
        // Immediately start playing BGM to get back on track faster
        bzrPlayNote(emuBzrBgm.song->notes[emuBzrBgm.note_index].note);
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
                    bzrPlayNote(track->song->notes[track->note_index].note);
                }
            }
            else
            {
                if (isActive)
                {
                    // Song is over
                    buzzernote = SILENCE;
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
 * @param samplesr The number of samples to read
 * @param samplesp The number of samples to write
 */
void EmuSoundCb(struct SoundDriver* sd, short* in, short* out, int samplesr, int samplesp)
{
    // Pass to microphone
    handleSoundInput(sd, in, out, samplesr, samplesp);

    // If this is an output callback, and there are samples to write
    if (samplesp && out)
    {
        // Keep track of our place in the wave
        static float placeInWave = 0;

        // If there is a note to play
        if (buzzernote)
        {
            // For each sample
            for (int i = 0; i < samplesp; i++)
            {
                // Write the sample
                out[i] = 1024 * sin(placeInWave);
                // Advance the place in the wave
                placeInWave += ((2 * M_PI * buzzernote) / ((float)SAMPLING_RATE));
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
            memset(out, 0, samplesp * 2);
            placeInWave = 0;
        }
    }
}
