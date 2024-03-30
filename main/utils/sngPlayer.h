/*! \file sngPlayer.h
 *
 * \section sngPlayer_design Design Philosophy
 *
 * This utility code is provided to play songs via the DAC speaker. Songs are loaded with spiffs_song.h.
 *
 * The 2024 Swadge had dual buzzers, each capable of producing a single square wave tone. The ::song_t struct was
 * designed to have two tracks (one per buzzer) that were each played over time. This code can be used to play those
 * same songs, but on a DAC speaker. It doesn't have limitations like only only being able to play one tone at a timer
 * (per-buzzer), but it also has no left, right, mono, or stereo options. It's all mono. It is also capable of
 * generating non-square waveforms. This code is currently designed to support playing up to two songs at a time
 * (::NUM_SONGS, to simulate background music and sound effects), each with two oscillators (::OSC_PER_SONG, to simulate
 * two buzzers). This may be extended in the future.
 *
 * By default, if ::swadgeMode_t.fnDacCb is left \c NULL, then the system will automatically use this song player.
 *
 * \section sngPlayer_usage Usage
 *
 * You don't need to call initSpkSongPlayer(), it's done by the system. There is no de-initializer.
 * sngPlayerFillBuffer() is used automatically by ::dacCallback() if ::swadgeMode_t.fnDacCb is left \c NULL .
 *
 * spkSongPlay() or spkSongPlayCb() can be used to start playing a song. spkSongStop() can be used to stop a song.
 *
 * spkSongPause() and spkSongResume() can be used to pause and resume a song. Unlike stopping, this saves the position
 * in the song being played.
 *
 * spkSongSave() and spkSongRestore() can also be used to pause and resume a song. These functions will return, and use,
 * the state, so different songs may be played between these function calls without losing state.
 *
 * \section sngPlayer_example Example
 *
 * The macros in soundFuncs.h should be used instead of these functions directly, so that the code remains
 * hardware-agnostic.
 *
 * \code{.c}
 * // Load a song
 * song_t ode_to_joy;
 * loadSong("ode.sng", &ode_to_joy, true);
 *
 * // Play the song at the 0th index
 * spkSongPlay(0, &ode_to_joy);
 *
 * // Pause and save the song
 * void* state = spkSongSave();
 *
 * // Restore and play the song
 * spkSongRestore(state);
 *
 * // Free the song when done
 * spkSongStop(true);
 * freeSong(&ode_to_joy);
 * \endcode
 */

#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "hdw-bzr.h"
#include "swSynth.h"

//==============================================================================
// Typedefs
//==============================================================================

/**
 * @brief Typedef for a function pointer which is called when a song finishes. May be registered with spkSongPlayCb()
 */
typedef void (*songFinishedCbFn)(void);

//==============================================================================
// Function Declarations
//==============================================================================

void initSpkSongPlayer(void);
void sngPlayerFillBuffer(uint8_t* samples, int16_t len);

void spkSongPlay(uint8_t sIdx, const song_t* song);
void spkSongPlayCb(uint8_t sIdx, const song_t* song, songFinishedCbFn cb);
void spkSongStop(bool resetTracks);

void spkSongPause(void);
void spkSongResume(void);

void* spkSongSave(void);
void spkSongRestore(void* data);

/*
void spkSetBgmVolume(uint16_t vol);
void spkSetSfxVolume(uint16_t vol);
uint16_t volLevelFromSetting(uint16_t setting);

void spkPlayNote(noteFrequency_t freq, buzzerPlayTrack_t track, uint16_t volume);
void spkStopNote(buzzerPlayTrack_t track);
*/
