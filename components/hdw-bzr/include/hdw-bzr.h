/*! \file hdw-bzr.h
 *
 * \section bzr_design Design Philosophy
 *
 * The buzzer is driven by the <a
 * href="https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/api-reference/peripherals/ledc.html">LEDC
 * peripheral</a>. This is usually used to generate a PWM signal to control the intensity of an LED, but here it
 * generates frequencies for the buzzer.
 *
 * A hardware timer is started which calls an interrupt every 5ms to check if the song should play the next note.
 *
 * This component manages two tracks, background music (bgm) and sound effects (sfx).
 * When bgm is playing, it may be interrupted by sfx.
 * If bgm and sfx are playing at the same time, both will progress through their respective notes, but only sfx will be
 * heard. This way, bgm keeps accurate time even with sfx.
 *
 * \section bzr_usage Usage
 *
 * You don't need to call initBuzzer() or deinitBuzzer(). The system does at the appropriate time.
 *
 * A ::musicalNote_t is a ::noteFrequency_t and a duration.
 * A ::song_t is a list of ::musicalNote_t that may be looped.
 *
 * The individual tracks may be muted with bzrSetBgmIsMuted() and bzrGetBgmIsMuted().
 *
 * A song can be played on a given track with either bzrPlayBgm() or bzrPlaySfx().
 * All tracks can be stopped at the same time with bzrStop().
 *
 * An individual note can be played with bzrPlayNote() or stopped with bzrStopNote().
 * This note is not on a specific track. It is useful for instrument modes, not for songs.
 *
 * MIDI files that are placed in the ./assets/ folder will be automatically converted to SNG files and loaded into the
 * SPIFFS filesystem. SNG files are lists of notes with durations and are compressed with Heatshrink compression. These
 * files can be loaded with loadSong() and must be freed with freeSong() when done.
 *
 * \section bzr_example Example
 *
 * \code{.c}
 * // Load a song
 * song_t ode_to_joy;
 * loadSong("ode.sng", &ode_to_joy);
 *
 * // Set the song to loop
 * ode_to_joy.shouldLoop = true;
 *
 * // Play the song as background music
 * bzrPlayBgm(&ode_to_joy);
 *
 * ...
 *
 * // Free the song when done
 * freeSong(&ode_to_joy);
 * \endcode
 */

#ifndef _BUZZER_H_
#define _BUZZER_H_

#include <stdbool.h>
#include <stdint.h>
#include <hal/gpio_types.h>
#include <driver/ledc.h>

// Frequencies of notes
typedef enum
{
    SILENCE    = 0,
    C_0        = 16,
    C_SHARP_0  = 17,
    D_0        = 18,
    D_SHARP_0  = 19,
    E_0        = 21,
    F_0        = 22,
    F_SHARP_0  = 23,
    G_0        = 25,
    G_SHARP_0  = 26,
    A_0        = 28,
    A_SHARP_0  = 29,
    B_0        = 31,
    C_1        = 33,
    C_SHARP_1  = 35,
    D_1        = 37,
    D_SHARP_1  = 39,
    E_1        = 41,
    F_1        = 44,
    F_SHARP_1  = 46,
    G_1        = 49,
    G_SHARP_1  = 52,
    A_1        = 55,
    A_SHARP_1  = 58,
    B_1        = 62,
    C_2        = 65,
    C_SHARP_2  = 69,
    D_2        = 73,
    D_SHARP_2  = 78,
    E_2        = 82,
    F_2        = 87,
    F_SHARP_2  = 93,
    G_2        = 98,
    G_SHARP_2  = 104,
    A_2        = 110,
    A_SHARP_2  = 117,
    B_2        = 123,
    C_3        = 131,
    C_SHARP_3  = 139,
    D_3        = 147,
    D_SHARP_3  = 156,
    E_3        = 165,
    F_3        = 175,
    F_SHARP_3  = 185,
    G_3        = 196,
    G_SHARP_3  = 208,
    A_3        = 220,
    A_SHARP_3  = 233,
    B_3        = 247,
    C_4        = 262,
    C_SHARP_4  = 277,
    D_4        = 294,
    D_SHARP_4  = 311,
    E_4        = 330,
    F_4        = 349,
    F_SHARP_4  = 370,
    G_4        = 392,
    G_SHARP_4  = 415,
    A_4        = 440,
    A_SHARP_4  = 466,
    B_4        = 494,
    C_5        = 523,
    C_SHARP_5  = 554,
    D_5        = 587,
    D_SHARP_5  = 622,
    E_5        = 659,
    F_5        = 698,
    F_SHARP_5  = 740,
    G_5        = 784,
    G_SHARP_5  = 831,
    A_5        = 880,
    A_SHARP_5  = 932,
    B_5        = 988,
    C_6        = 1047,
    C_SHARP_6  = 1109,
    D_6        = 1175,
    D_SHARP_6  = 1245,
    E_6        = 1319,
    F_6        = 1397,
    F_SHARP_6  = 1480,
    G_6        = 1568,
    G_SHARP_6  = 1661,
    A_6        = 1760,
    A_SHARP_6  = 1865,
    B_6        = 1976,
    C_7        = 2093,
    C_SHARP_7  = 2217,
    D_7        = 2349,
    D_SHARP_7  = 2489,
    E_7        = 2637,
    F_7        = 2794,
    F_SHARP_7  = 2960,
    G_7        = 3136,
    G_SHARP_7  = 3322,
    A_7        = 3520,
    A_SHARP_7  = 3729,
    B_7        = 3951,
    C_8        = 4186,
    C_SHARP_8  = 4435,
    D_8        = 4699,
    D_SHARP_8  = 4978,
    E_8        = 5274,
    F_8        = 5588,
    F_SHARP_8  = 5920,
    G_8        = 6272,
    G_SHARP_8  = 6645,
    A_8        = 7040,
    A_SHARP_8  = 7459,
    B_8        = 7902,
    C_9        = 8372,
    C_SHARP_9  = 8870,
    D_9        = 9397,
    D_SHARP_9  = 9956,
    E_9        = 10548,
    F_9        = 11175,
    F_SHARP_9  = 11840,
    G_9        = 12544,
    G_SHARP_9  = 13290,
    A_9        = 14080,
    A_SHARP_9  = 14917,
    B_9        = 15804,
    C_10       = 16744,
    C_SHARP_10 = 17740,
    D_10       = 18795,
    D_SHARP_10 = 19912,
    E_10       = 21096,
    F_10       = 22351,
    F_SHARP_10 = 23680,
    G_10       = 25088,
    G_SHARP_10 = 26580,
    A_10       = 28160,
    A_SHARP_10 = 29834,
    B_10       = 31609
} noteFrequency_t;

typedef struct
{
    /// Note frequency, in Hz
    noteFrequency_t note;
    /// Note duration, in ms
    uint32_t timeMs;
} musicalNote_t;

typedef struct
{
    /// true if the song should loop, false if it should play once
    uint32_t shouldLoop;
    /// The number of notes in this song
    uint32_t numNotes;
    /// The note index to restart at, if looping
    uint32_t loopStartNote;
    /// An array of notes in the song
    musicalNote_t* notes;
} song_t;

void initBuzzer(gpio_num_t bzrGpio, ledc_timer_t _ledcTimer, ledc_channel_t _ledcChannel, bool _isBgmMuted,
                bool _isSfxMuted);
void deinitBuzzer(void);
void bzrSetBgmIsMuted(bool _isBgmMuted);
void bzrGetBgmIsMuted(bool _isSfxMuted);
void bzrPlayBgm(const song_t* song);
void bzrPlaySfx(const song_t* song);
void bzrStop(void);
void bzrPlayNote(noteFrequency_t freq);
void bzrStopNote(void);

#endif