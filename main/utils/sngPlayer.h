#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "hdw-bzr.h"
#include "swSynth.h"

//==============================================================================
// Function Declarations
//==============================================================================

void initSpkSongPlayer(void);
void spkPlaySong(uint8_t sIdx, song_t* song);
void sngPlayerFillBuffer(uint8_t* samples, int16_t len);