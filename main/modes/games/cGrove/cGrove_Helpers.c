/**
 * @file cGrove_Helpers.c
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief Helpers used in several parts of the program
 * @version 0.1
 * @date 2024-05-07
 * 
 * @copyright Copyright (c) 2024
 * 
 */

// Includes
//==============================================================================
#include "cGrove_Helpers.h"

// Functions
//==============================================================================
int8_t cGroveCustomSelectionWrap(buttonEvt_t evt, int8_t maxBound, int8_t *selection)
{
    // Increment
    if (evt.state & PB_UP) {
        *selection -= 1;
    } else {
        *selection += 1;
    }
    // Reset if out of bounds
    if (*selection >= maxBound) {
        *selection = 0;
    } else if (*selection < 0) {
        *selection = maxBound - 1;
    }
    return *selection;
}