/*! \file hdw-bzr.h
 *
 * \section bzr_design Design Philosophy
 *
 * The buzzers are driven by the <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.1.1/esp32s2/api-reference/peripherals/ledc.html">LEDC
 * peripheral</a>. This is usually used to generate a PWM signal to control the intensity of an LED, but here it
 * generates frequencies for the buzzers.
 *
 * A hardware timer is started which calls an interrupt every 5ms to check if the song should play the next note, so no
 * note can be shorter than 5ms.
 *
 * This component manages two physical buzzers, each with two tracks, background music (BGM) and sound effects (SFX).
 * When BGM is playing, it may be interrupted by SFX. SFX will not interrupt BGM.
 * If BGM and SFX are playing at the same time, both will progress through their respective notes, but only SFX will be
 * heard. This way, BGM keeps accurate time even when SFX is playing.
 *
 * Songs may be played on one or more buzzer. If the ::song_t has two tracks, the first one will play on the left buzzer
 * and the second one will play on the right one. This cannot be changed. If the ::song_t has one track, it may be
 * played on the left buzzer, the right buzzer, or both simultaneously. If a one-track SFX interrupts a two-track BGM,
 * it may interrupt on just the left buzzer, just the right buzzer, or both.
 *
 * A ::musicalNote_t is a ::noteFrequency_t and a duration.
 * A ::songTrack_t is a list of ::musicalNote_t which are played in sequence.
 * A ::song_t is one or two ::songTrack_t, each of identical total time length, which may be looped.
 *
 * \section bzr_usage Usage
 *
 * You don't need to call initBuzzer() or deinitBuzzer(). The system does at the appropriate time.
 *
 * The BGM and SFX may have their volume individually adjusted with bzrSetBgmVolume() and bzrSetSfxVolume().
 * setBgmVolumeSetting() and setSfxVolumeSetting() should be called instead if the volume change should be persistent
 * through reboots. Setting the volume to 0 will mute it.
 *
 * A song can be played on with either bzrPlayBgm() or bzrPlaySfx().
 * Both BGM and SFX can be stopped at the same time with bzrStop().
 *
 * An individual note can be played with bzrPlayNote() or stopped with bzrStopNote().
 * This note is not considered BGM or SFX. It is useful for instrument modes, not for songs.
 *
 * MIDI files that are placed in the ./assets/ folder will be automatically converted to SNG files and loaded into the
 * SPIFFS filesystem. MIDI files may have up to two tracks of non-overlapping notes. Any more tracks will be ignored.
 * SNG files are lists of notes with durations and are compressed with Heatshrink compression. These files can be loaded
 * with loadSong() and must be freed with freeSong() when done.
 *
 * \section bzr_example Example
 *
 * \code{.c}
 * // Load a song
 * song_t ode_to_joy;
 * loadSong("ode.sng", &ode_to_joy, true);
 *
 * // Set the song to loop
 * ode_to_joy.shouldLoop = true;
 *
 * // Play the song as background music
 * bzrPlayBgm(&ode_to_joy, BZR_STEREO);
 *
 * ...
 *
 * // Free the song when done
 * bzrStop(true);
 * freeSong(&ode_to_joy);
 * \endcode
 */

#ifndef _BUZZER_H_
#define _BUZZER_H_

#include <stdbool.h>
#include <stdint.h>
#include <hal/gpio_types.h>
#include <driver/ledc.h>

/** @brief The number of physical buzzers */
#define NUM_BUZZERS 2

#define MAX_VOLUME 13

/**
 * @brief Frequencies for all the notes, in hertz.
 *
 * Some will sound better than others.
 */
typedef enum
{
    SILENCE    = 0,     ///< Silence
    C_0        = 16,    ///< C0
    C_SHARP_0  = 17,    ///< C#0
    D_0        = 18,    ///< D0
    D_SHARP_0  = 19,    ///< D#0
    E_0        = 21,    ///< E0
    F_0        = 22,    ///< F0
    F_SHARP_0  = 23,    ///< F#0
    G_0        = 25,    ///< G0
    G_SHARP_0  = 26,    ///< G#0
    A_0        = 28,    ///< A0
    A_SHARP_0  = 29,    ///< A#0
    B_0        = 31,    ///< B0
    C_1        = 33,    ///< C1
    C_SHARP_1  = 35,    ///< C#1
    D_1        = 37,    ///< D1
    D_SHARP_1  = 39,    ///< D#1
    E_1        = 41,    ///< E1
    F_1        = 44,    ///< F1
    F_SHARP_1  = 46,    ///< F#1
    G_1        = 49,    ///< G1
    G_SHARP_1  = 52,    ///< G#1
    A_1        = 55,    ///< A1
    A_SHARP_1  = 58,    ///< A#1
    B_1        = 62,    ///< B1
    C_2        = 65,    ///< C2
    C_SHARP_2  = 69,    ///< C#2
    D_2        = 73,    ///< D2
    D_SHARP_2  = 78,    ///< D#2
    E_2        = 82,    ///< E2
    F_2        = 87,    ///< F2
    F_SHARP_2  = 93,    ///< F#2
    G_2        = 98,    ///< G2
    G_SHARP_2  = 104,   ///< G#2
    A_2        = 110,   ///< A2
    A_SHARP_2  = 117,   ///< A#2
    B_2        = 123,   ///< B2
    C_3        = 131,   ///< C3
    C_SHARP_3  = 139,   ///< C#3
    D_3        = 147,   ///< D3
    D_SHARP_3  = 156,   ///< D#3
    E_3        = 165,   ///< E3
    F_3        = 175,   ///< F3
    F_SHARP_3  = 185,   ///< F#3
    G_3        = 196,   ///< G3
    G_SHARP_3  = 208,   ///< G#3
    A_3        = 220,   ///< A3
    A_SHARP_3  = 233,   ///< A#3
    B_3        = 247,   ///< B3
    C_4        = 262,   ///< C4
    C_SHARP_4  = 277,   ///< C#4
    D_4        = 294,   ///< D4
    D_SHARP_4  = 311,   ///< D#4
    E_4        = 330,   ///< E4
    F_4        = 349,   ///< F4
    F_SHARP_4  = 370,   ///< F#4
    G_4        = 392,   ///< G4
    G_SHARP_4  = 415,   ///< G#4
    A_4        = 440,   ///< A4
    A_SHARP_4  = 466,   ///< A#4
    B_4        = 494,   ///< B4
    C_5        = 523,   ///< C5
    C_SHARP_5  = 554,   ///< C#5
    D_5        = 587,   ///< D5
    D_SHARP_5  = 622,   ///< D#5
    E_5        = 659,   ///< E5
    F_5        = 698,   ///< F5
    F_SHARP_5  = 740,   ///< F#5
    G_5        = 784,   ///< G5
    G_SHARP_5  = 831,   ///< G#5
    A_5        = 880,   ///< A5
    A_SHARP_5  = 932,   ///< A#5
    B_5        = 988,   ///< B5
    C_6        = 1047,  ///< C6
    C_SHARP_6  = 1109,  ///< C#6
    D_6        = 1175,  ///< D6
    D_SHARP_6  = 1245,  ///< D#6
    E_6        = 1319,  ///< E6
    F_6        = 1397,  ///< F6
    F_SHARP_6  = 1480,  ///< F#6
    G_6        = 1568,  ///< G6
    G_SHARP_6  = 1661,  ///< G#6
    A_6        = 1760,  ///< A6
    A_SHARP_6  = 1865,  ///< A#6
    B_6        = 1976,  ///< B6
    C_7        = 2093,  ///< C7
    C_SHARP_7  = 2217,  ///< C#7
    D_7        = 2349,  ///< D7
    D_SHARP_7  = 2489,  ///< D#7
    E_7        = 2637,  ///< E7
    F_7        = 2794,  ///< F7
    F_SHARP_7  = 2960,  ///< F#7
    G_7        = 3136,  ///< G7
    G_SHARP_7  = 3322,  ///< G#7
    A_7        = 3520,  ///< A7
    A_SHARP_7  = 3729,  ///< A#7
    B_7        = 3951,  ///< B7
    C_8        = 4186,  ///< C8
    C_SHARP_8  = 4435,  ///< C#8
    D_8        = 4699,  ///< D8
    D_SHARP_8  = 4978,  ///< D#8
    E_8        = 5274,  ///< E8
    F_8        = 5588,  ///< F8
    F_SHARP_8  = 5920,  ///< F#8
    G_8        = 6272,  ///< G8
    G_SHARP_8  = 6645,  ///< G#8
    A_8        = 7040,  ///< A8
    A_SHARP_8  = 7459,  ///< A#8
    B_8        = 7902,  ///< B8
    C_9        = 8372,  ///< C9
    C_SHARP_9  = 8870,  ///< C#9
    D_9        = 9397,  ///< D9
    D_SHARP_9  = 9956,  ///< D#9
    E_9        = 10548, ///< E9
    F_9        = 11175, ///< F9
    F_SHARP_9  = 11840, ///< F#9
    G_9        = 12544, ///< G9
    G_SHARP_9  = 13290, ///< G#9
    A_9        = 14080, ///< A9
    A_SHARP_9  = 14917, ///< A#9
    B_9        = 15804, ///< B9
    C_10       = 16744, ///< C10
    C_SHARP_10 = 17740, ///< C#10
    D_10       = 18795, ///< D10
    D_SHARP_10 = 19912, ///< D#10
    E_10       = 21096, ///< E10
    F_10       = 22351, ///< F10
    F_SHARP_10 = 23680, ///< F#10
    G_10       = 25088, ///< G10
    G_SHARP_10 = 26580, ///< G#10
    A_10       = 28160, ///< A10
    A_SHARP_10 = 29834, ///< A#10
    B_10       = 31609  ///< B10
} noteFrequency_t;

/**
 * @brief Enum for which buzzer a note should play out of
 */
typedef enum
{
    BZR_LEFT,  ///< Play out of the left buzzer
    BZR_RIGHT, ///< Play out of the right buzzer
    BZR_STEREO ///< Play out of both buzzers
} buzzerPlayTrack_t;

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

void initBuzzer(gpio_num_t bzrGpioL, ledc_timer_t ledcTimerL, ledc_channel_t ledcChannelL, gpio_num_t bzrGpioR,
                ledc_timer_t ledcTimerR, ledc_channel_t ledcChannelR, uint16_t _bgmVolume, uint16_t _sfxVolume);
void deinitBuzzer(void);
void bzrSetBgmVolume(uint16_t vol);
void bzrSetSfxVolume(uint16_t vol);
void bzrPlayBgm(const song_t* song, buzzerPlayTrack_t track);
void bzrPlaySfx(const song_t* song, buzzerPlayTrack_t track);
void bzrStop(bool resetTracks);
void bzrPlayNote(noteFrequency_t freq, buzzerPlayTrack_t track, uint16_t volume);
void bzrStopNote(buzzerPlayTrack_t track);
void bzrPause(void);
void bzrResume(void);
void* bzrSave(void);
void bzrRestore(void* data);

#endif