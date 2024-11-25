/**
 * @file cg_sparDraw.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Draws the Chowa Garden Spar
 * @version 0.1
 * @date 2024-09-19
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

void cg_drawSparSplash(cGrove_t* cg, int64_t elapsedUs);
void cg_drawSparRecord(cGrove_t* cg);
void cg_drawSparTournamentSetup(cGrove_t* cg);
void cg_drawSparPrivateSetup(cGrove_t* cg);
void cg_drawSparGuestSetup(cGrove_t* cg);
void cg_drawSparMatchPrep(cGrove_t* cg);
void cg_drawSparMatch(cGrove_t* cg, int64_t elapsedUs);