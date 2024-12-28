/**
 * @file cg_Garden.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief The main interation area with the Chowa
 * @version 1.0
 * @date 2024-09-07
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
// Externs
//==============================================================================

extern const char* cgNVSKeys[];

//==============================================================================
// Function declarations
//==============================================================================

void cg_initGrove(cGrove_t* cg);
void cg_deInitGrove(cGrove_t* cg);
void cg_runGrove(cGrove_t* cg, int64_t elapsedUS);
void cg_clearGroveNVSData(void);
