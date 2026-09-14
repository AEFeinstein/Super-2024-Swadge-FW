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

// NOTE: These have been included purely because VSC's intellisense can't find the defines inside of
// c_cpp_properties.json for some reason
#ifndef TFT_HEIGHT
    #define TFT_HEIGHT 240
#endif
#ifndef TFT_WIDTH
    #define TFT_WIDTH 280
#endif

//==============================================================================
// Function Definitions
//==============================================================================

/**
 * @brief Handles moving it 2D with a wrap in both x and y directions
 *
 * @param evt button_evt event to check
 * @param selection Current value selected
 * @param colLim How wide the columns are
 * @param maxVal The limit of the array
 * @return int New value of selection
 */
int ciMenu2DNavigate(buttonEvt_t* evt, int selection, int colLim, int maxVal);