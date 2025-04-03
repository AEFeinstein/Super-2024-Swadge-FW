/**
 * @file cg_chowa.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Pet behavior and appearance
 * @version 1.0
 * @date 2024-09-08
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

void cg_initChowaWSGs(cGrove_t* cg);
void cg_deInitChowaWSGs(cGrove_t* cg);
wsg_t* cg_getChowaWSG(cGrove_t* cg, cgChowa_t* c, cgChowaAnimIdx_t anim, int8_t idx);