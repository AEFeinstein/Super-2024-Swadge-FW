#ifndef _MODE_PLATFORMER_H_
#define _MODE_PLATFORMER_H_
//==============================================================================
// Includes
//==============================================================================

#include "pango_typedef.h"
#include "swadge2024.h"

/*==============================================================================
 * Constants
 *============================================================================*/

#define NUM_PLATFORMER_HIGH_SCORES 5

extern const char pangoName[];

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint32_t scores[NUM_PLATFORMER_HIGH_SCORES];
    char initials[NUM_PLATFORMER_HIGH_SCORES][3];
} pangoHighScores_t;

typedef struct
{
    uint8_t maxLevelIndexUnlocked;
    bool gameCleared;
    bool oneCreditCleared;
    bool bigScore;
    bool fastTime;
    bool biggerScore;
} pangoUnlockables_t;

//==============================================================================
// Prototypes
//==============================================================================

void updateGame(pango_t* pango, int64_t elapsedUs);
void updateTitleScreen(pango_t* pango, int64_t elapsedUs);

extern swadgeMode_t pangoMode;

#endif
