#pragma once

#include "sequencerMode.h"

#define NUM_PIANO_KEYS 88

void measureSequencerGrid(sequencerVars_t* sv);
void sequencerGridButton(sequencerVars_t* sv, buttonEvt_t* evt);
void sequencerGridTouch(sequencerVars_t* sv);

void runSequencerTimers(sequencerVars_t* sv, int32_t elapsedUs);
void drawSequencerGrid(sequencerVars_t* sv, int32_t elapsedUs);
