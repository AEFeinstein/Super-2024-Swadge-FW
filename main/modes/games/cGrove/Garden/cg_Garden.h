/**
 * @file cg_Garden.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief The main interation area with the Chowa
 * @version 0.1
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
// Defines
//==============================================================================

#define CG_CURSOR_SPEED 16

//==============================================================================
// Function declarations
//==============================================================================

/**
 * @brief Initialize the Garden mode
 * 
 * @param cg Game Object
 */
void cgInitGarden(cGrove_t* cg);

/**
 * @brief Main loop ofr Garden mode
 * 
 * @param cg Game Object
 */
void cgRunGarden(cGrove_t* cg);