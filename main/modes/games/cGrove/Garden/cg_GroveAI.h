/**
 * @file cg_GroveAI.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Chowa AI in the garden
 * @version 1.0
 * @date 2024-10-13
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

void cg_GroveAI(cGrove_t* cg, cgGroveChowa_t* chowa, int64_t elapsedUs);
void cg_GroveEggAI(cGrove_t* cg, int64_t elapsedUs);