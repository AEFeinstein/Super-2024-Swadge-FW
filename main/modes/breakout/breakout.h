#ifndef _BREAKOUT_MODE_H_
#define _BREAKOUT_MODE_H_

#include "swadge2024.h"

extern swadgeMode_t breakoutMode;

/*==============================================================================
 * Defines
 *============================================================================*/

#define NUM_BREAKOUT_HIGH_SCORES 5

//==============================================================================
// Structs
//==============================================================================

typedef struct {
    uint32_t scores[NUM_BREAKOUT_HIGH_SCORES];
    char initials[NUM_BREAKOUT_HIGH_SCORES][3];
} breakoutHighScores_t;

#endif