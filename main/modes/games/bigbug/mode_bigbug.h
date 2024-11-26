#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "swadge2024.h"
#include "fp_math.h"

//==============================================================================
// Defines
//==============================================================================

#ifndef _MODE_BIGBUG_H_
    #define _MODE_BIGBUG_H_

void bb_setupMidi(void);

extern swadgeMode_t bigbugMode;

extern heatshrink_decoder* bb_hsd;
extern uint8_t* bb_decodeSpace;

extern const char BB_TAG[];

#endif