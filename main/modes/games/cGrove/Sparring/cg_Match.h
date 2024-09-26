/**
 * @file cg_Match.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Provide the individual match implementation for Chowa Grove spars
 * @version 0.1
 * @date 2024-09-22
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "cg_Typedef.h"

//==============================================================================
// Functions
//==============================================================================

void cg_initMatch(cGrove_t* cg, char* matchName, cgChowa_t* player1Chowa, cgChowa_t* player2Chowa, int8_t round,
                  int16_t maxTime);
void cg_runSparMatch(cGrove_t* cg, int64_t elapsedUs);