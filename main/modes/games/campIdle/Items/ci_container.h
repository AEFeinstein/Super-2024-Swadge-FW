#pragma once

//==============================================================================
// Include
//==============================================================================

#include "ci_genericData.h"
#include "swadge.h"

//==============================================================================
// Function declarations
//==============================================================================

void ciInitContainer(ciContainer_t* cont, int maxSlots, int weightLim, int sizeLim);

void ciFreeContainer(ciContainer_t* cont);

int ciAddItemToContainer(ciContainer_t* cont, ciItemIdx_t item, int qty);

int ciRemoveFromContainer(ciContainer_t* cont, ciItemIdx_t item, int qty);

int ciFindItemInContainer(ciContainer_t* cont, ciItemIdx_t item, int* slotLoc);

bool ciTransferBetweenContainers(ciContainer_t* cont1, ciContainer_t* cont2, ciItemIdx_t item, int qty);

void ciDrawContainer(ciCampData_t* ccd, ciContainer_t* cont, int x, int y, int maxCols, int maxRows, font_t* fnt);
