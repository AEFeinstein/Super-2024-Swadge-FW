#pragma once

#include "mode_swadgeHero.h"

#define NUM_FAIL_METER_SAMPLES 200

void shLoadSong(shVars_t* sh, const char* midi, const char* chart);
uint32_t shLoadChartData(shVars_t* sh, const uint8_t* data, size_t size);
void shGameInput(shVars_t* sh, buttonEvt_t* evt);
bool shRunTimers(shVars_t* sh, uint32_t elapsedUs);
void shDrawGame(shVars_t* sh);
