#ifndef _MEGA_PULSE_EX_H_
#define _MEGA_PULSE_EX_H_
//==============================================================================
// Includes
//==============================================================================

#include "mega_pulse_ex_typedef.h"
#include "swadge2024.h"

/*==============================================================================
 * Constants
 *============================================================================*/

#define NUM_PLATFORMER_HIGH_SCORES 5

extern const char platformerName[];

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    uint32_t scores[NUM_PLATFORMER_HIGH_SCORES];
    char initials[NUM_PLATFORMER_HIGH_SCORES][3];
} platformerHighScores_t;

typedef struct
{
    uint8_t levelsCleared;
    bool gameCleared;
    bool oneCreditCleared;
    bool bigScore;
    bool fastTime;
    bool biggerScore;
} platformerUnlockables_t;

//==============================================================================
// Prototypes
//==============================================================================

void updateGame(platformer_t* platformer);
void updateTitleScreen(platformer_t* platformer);

extern swadgeMode_t modePlatformer;

#endif
