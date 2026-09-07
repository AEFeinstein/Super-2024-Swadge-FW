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

int32_t ciAddToInv(ciCampData_t* ccd, ciItemIdx_t item, int qty);

bool ciRemoveFromInv(ciCampData_t* ccd, ciItemIdx_t item, int qty);

void ciDrawItemPanel(ciCampData_t* ccd, ciItemIdx_t idx);

void ciDrawItemIcon(ciCampData_t* ccd, ciItemIdx_t idx, int xStart, int yStart, int qty, bool selected, bool showQty);