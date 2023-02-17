//==============================================================================
// Includes
//==============================================================================

#include "hdw-bzr.h"
#include "emu_main.h"

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
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Set the buzzer's bgm mute status
 *
 * @param _isBgmMuted True if background music is muted, false otherwise
 */
void bzrSetBgmIsMuted(bool _isBgmMuted)
{
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Set the buzzer's sfx mute status
 *
 * @param _isSfxMuted True if sound effects are muted, false otherwise
 */
void bzrGetBgmIsMuted(bool _isSfxMuted)
{
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Start playing a background music on the buzzer. This has lower priority
 * than sound effects
 *
 * @param song The song to play as a sequence of notes
 */
void bzrPlayBgm(const song_t* song)
{
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Start playing a sound effect on the buzzer. This has higher priority
 * than background music
 *
 * @param song The song to play as a sequence of notes
 */
void bzrPlaySfx(const song_t* song)
{
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Stop the buzzer from playing anything
 */
void bzrStop(void)
{
    WARN_UNIMPLEMENTED();
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
    WARN_UNIMPLEMENTED();
}

/**
 * @brief Stop playing a single note on the buzzer
 * This has IRAM_ATTR because it may be called from an interrupt
 */
void bzrStopNote(void)
{
    WARN_UNIMPLEMENTED();
}
