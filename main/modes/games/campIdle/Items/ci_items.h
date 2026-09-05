#pragma once

//==============================================================================
// Includes
//==============================================================================

// CI
#include "ci_genericData.h"
#include "wsg.h"

//==============================================================================
// Defines
//==============================================================================

#define ICON_WIDTH           40
#define ICON_HEIGHT          54

//==============================================================================
// Function Definitions
//==============================================================================

// Initialization

void ciInitInventory(ciCampData_t* ccd);

void ciFreeInventory(ciCampData_t* ccd);

void ciDrawItemPanel(ciCampData_t* ccd, int idx);

void ciDrawItemIcon(ciCampData_t* ccd, int idx, int xStart, int yStart, int qty, bool selected, bool showQty);