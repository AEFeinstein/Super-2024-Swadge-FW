#pragma once

#include "sequencerMode.h"

void measureSequencerGrid(sequencerVars_t* sv);
void sequencerGridButton(sequencerVars_t* sv, buttonEvt_t* evt);
void drawSequencerGrid(sequencerVars_t* sv, int32_t elapsedUs);
