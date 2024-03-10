#pragma once

#include "hdw-bzr.h"
#include "swSynth.h"

void playSongSpk(song_t* song);
void sngPlayerFillBuffer(uint8_t* samples, int16_t len);