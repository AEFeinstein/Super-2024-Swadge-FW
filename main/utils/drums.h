#pragma once
#include "midiPlayer.h"

int8_t defaultDrumkitFunc(percussionNote_t drum, uint32_t idx, bool* done, uint32_t scratch[4], void* data);
int8_t donutDrumkitFunc(percussionNote_t drum, uint32_t idx, bool* done, uint32_t scratch[4], void* data);