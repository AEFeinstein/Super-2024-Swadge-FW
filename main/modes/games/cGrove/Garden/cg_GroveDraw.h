/**
 * @file cg_GroveDraw.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Drawing functions for the Grove mode of Chowa Grove
 * @version 0.1
 * @date 2024-10-09
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
// Function declarations
//==============================================================================

void cg_groveDrawField(cGrove_t* cg, int64_t elapsedUs);
void cg_groveDrawShop(cGrove_t* cg);
void cg_groveDrawInv(cGrove_t* cg);
void cg_groveDrawStats(cGrove_t* cg);
void cg_groveDrawAbandon(cGrove_t* cg);
void cg_drawGroveTutorial(cGrove_t* cg);
bool cg_checkFull(cGrove_t* cg);