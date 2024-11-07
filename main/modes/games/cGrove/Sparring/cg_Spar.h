/**
 * @file cg_spar.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Provides the sparring implementation for Chowa Grove
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

void cg_initSpar(cGrove_t* cg);
void cg_deInitSpar(void);
void cg_runSpar(int64_t elapsedUs);
