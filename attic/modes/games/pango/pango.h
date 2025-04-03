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
    uint8_t character[NUM_PLATFORMER_HIGH_SCORES];
} pangoHighScores_t;

typedef struct
{
    uint8_t maxLevelIndexUnlocked;
    bool gameCleared;
    bool oneCreditCleared;
    uint32_t caravanHighScore;
} pangoUnlockables_t;

//==============================================================================
// Prototypes
//==============================================================================

void updateGame(pango_t* pango, int64_t elapsedUs);
void updateTitleScreen(pango_t* pango, int64_t elapsedUs);

extern swadgeMode_t pangoMode;

#endif
