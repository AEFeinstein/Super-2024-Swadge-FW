#pragma once

#include "mode_swadgeHero.h"

#define NUM_FAIL_METER_SAMPLES 200

typedef struct
{
    int32_t timing;
    const char* label;
} shTimingGrade_t;

extern const shTimingGrade_t timings[6]; // TODO don't hardcode

void shLoadSong(shVars_t* sh, const shSong_t* song, shDifficulty_t difficulty);
uint32_t shLoadChartData(shVars_t* sh, const uint8_t* data, size_t size);
void shGameInput(shVars_t* sh, buttonEvt_t* evt);
bool shRunTimers(shVars_t* sh, uint32_t elapsedUs);
void shDrawGame(shVars_t* sh);
