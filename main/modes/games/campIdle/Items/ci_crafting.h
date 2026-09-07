#pragma once

//==============================================================================
// Include
//==============================================================================

#include "ci_genericData.h"

//==============================================================================
// Function Definitions
//==============================================================================

void ciInitCraftSelection(ciCampData_t* ccd);

void ciInitCraft(ciCampData_t* ccd);

void ciRunCraftSelection(ciCampData_t* ccd);

bool ciRunCraft(ciCampData_t* ccd, int64_t elapsedUs);

void ciInitCraftTimer(ciCampData_t* ccd);

void ciCraft(ciCampData_t* ccd);