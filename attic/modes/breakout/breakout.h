#ifndef _BREAKOUT_MODE_H_
#define _BREAKOUT_MODE_H_

#include "swadge2024.h"

extern swadgeMode_t breakoutMode;

/*==============================================================================
 * Defines
 *============================================================================*/

#define NUM_BREAKOUT_HIGH_SCORES 5

extern const char breakoutName[];

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint32_t scores[NUM_BREAKOUT_HIGH_SCORES];
    char initials[NUM_BREAKOUT_HIGH_SCORES][3];
} breakoutHighScores_t;

typedef struct
{
    uint8_t maxLevelIndexUnlocked;
    bool gameCleared;
    /*bool oneCreditCleared;
    bool bigScore;
    bool fastTime;
    bool biggerScore;*/
} breakoutUnlockables_t;

#endif