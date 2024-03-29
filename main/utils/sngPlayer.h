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
 * @brief TODO
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
