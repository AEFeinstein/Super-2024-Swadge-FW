#ifndef _MODE_SHOOTER_H_
#define _MODE_SHOOTER_H_

#include "swadge2024.h"

typedef enum
{
    RC_EASY,
    RC_MED,
    RC_HARD,
    RC_NUM_DIFFICULTIES
} shooterDifficulty_t;

typedef enum
{
    RC_MAP_S,
    RC_MAP_M,
    RC_MAP_L,
    RC_NUM_MAPS
} shooterMap_t;

typedef struct __attribute__((aligned(4)))
{
    uint16_t kills;
    uint32_t tElapsedUs;
} shooterScore_t;

#define RC_NUM_SCORES 4

typedef struct __attribute__((aligned(4)))
{
    shooterScore_t scores[RC_NUM_MAPS][RC_NUM_DIFFICULTIES][RC_NUM_SCORES];
} shooterScores_t;

extern swadgeMode_t shooterMode;

#endif