#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "ci_genericData.h"

//==============================================================================
// Defines
//==============================================================================

#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))
#define SET_BIT(var, pos)   (var) |= (1 << (pos))

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Handles moving it 2D with a wrap in both x and y directions
 *
 * @param evt button_evt event to check
 * @param selection Current value selected
 * @param colLim How wide the colums are
 * @param maxVal The limit of the array
 * @return int New value of selection
 */
int ciMenu2DNavigate(buttonEvt_t* evt, int selection, int colLim, int maxVal);