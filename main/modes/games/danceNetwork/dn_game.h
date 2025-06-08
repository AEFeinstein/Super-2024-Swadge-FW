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
void dn_UpdateGame(dn_gameData_t* gameData, uint32_t elapsedUs);
void dn_DrawGame(dn_gameData_t* gameData);
void dn_DrawTiles(dn_gameData_t* gameData);

void dn_BeginGame(dn_gameData_t* gameData);