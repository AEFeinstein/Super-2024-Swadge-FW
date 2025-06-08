#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "danceNetwork.h"

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Function Declarations
//==============================================================================

void dn_HandleGameInput(dn_gameData_t* gameData, buttonEvt_t* evt);
void dn_DrawGame(dn_gameData_t* gameData, uint32_t elapsedUs);
void dn_DrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t m, paletteColor_t color);

void dn_BeginGame(dn_gameData_t* gameData);