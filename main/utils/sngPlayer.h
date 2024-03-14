#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "hdw-spk.h"
#include "swSynth.h"

//==============================================================================
// Typedefs
//==============================================================================

typedef void (*songFinishedCbFn)(void);

//==============================================================================
// Function Declarations
//==============================================================================

void initSpkSongPlayer(void);
void spkPlaySong(uint8_t sIdx, song_t* song);
void spkPlaySongCb(uint8_t sIdx, song_t* song, songFinishedCbFn cb);
void sngPlayerFillBuffer(uint8_t* samples, int16_t len);
void spkStopSong(bool resetTracks);

/*
void initBuzzer(gpio_num_t spkGpioL, ledc_timer_t ledcTimerL, ledc_channel_t ledcChannelL, gpio_num_t spkGpioR,
                ledc_timer_t ledcTimerR, ledc_channel_t ledcChannelR, uint16_t _bgmVolume, uint16_t _sfxVolume);
void deinitBuzzer(void);
void spkCheckSongDone(void);

void spkSetBgmVolume(uint16_t vol);
void spkSetSfxVolume(uint16_t vol);
uint16_t volLevelFromSetting(uint16_t setting);

void spkPlayBgm(const song_t* song, buzzerPlayTrack_t track);
void spkPlayBgmCb(const song_t* song, buzzerPlayTrack_t track, songFinishedCbFn cbFn);
void spkPlaySfx(const song_t* song, buzzerPlayTrack_t track);
void spkPlaySfxCb(const song_t* song, buzzerPlayTrack_t track, songFinishedCbFn cbFn);
void spkStop(bool resetTracks);

bool spkPause(void);
void spkResume(void);

void* spkSave(void);
void spkRestore(void* data);

void spkPlayNote(noteFrequency_t freq, buzzerPlayTrack_t track, uint16_t volume);
void spkStopNote(buzzerPlayTrack_t track);
*/