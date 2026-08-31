#pragma once

//==============================================================================
// Includes
//==============================================================================

// CI
#include "ci_genericData.h"
#include "wsg.h"

//==============================================================================
// Function Definitions
//==============================================================================

// Initialization

void ciInitInventory(ciCampData_t* ccd);

void ciFreeInventory(ciCampData_t* ccd);

void ciDrawItemPanel(ciCampData_t* ccd, int idx);

void ciDrawItemIcon(ciCampData_t* ccd, int idx, int xStart, int yStart, bool selected);