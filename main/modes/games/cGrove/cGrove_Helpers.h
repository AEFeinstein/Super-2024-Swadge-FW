#ifndef _CGROVE_HELPERS_H_
#define _CGROVE_HELPERS_H_

#include "swadge2024.h"

/**
 * @brief Adjusts a value to stay within set bounds
 * 
 * @param evt Button event to use
 * @param maxBound Number of items in list before wrapping
 * @param selection Unsigned number that is set to new value based on input
 * @return int8_t Returns same value of selection
 */
int8_t cGroveCustomSelectionWrap(buttonEvt_t, int8_t, int8_t*);

#endif