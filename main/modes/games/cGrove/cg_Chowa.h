/**
 * @file cg_chowa.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Pet behavior and appearance
 * @version 0.1
 * @date 2024-09-08
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#pragma once

// Includes
#include "mode_cGrove.h"

//==============================================================================
// Function declarations
//==============================================================================

void cgInitChowa(cGrove_t* cg, int8_t idx, paletteColor_t* colors);

void cgDrawChowa(cGrove_t* cg, int8_t idx, vec_t offset);