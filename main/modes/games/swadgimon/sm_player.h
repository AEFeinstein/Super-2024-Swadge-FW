#ifndef _SM_PLAYER_H_
#define _SM_PLAYER_H_

#include <stdbool.h>
#include "sm_constants.h"

// Definitions of player data
struct {
    int64_t timePlayed;
    uint32_t money;
    uint16_t trainerId;
    bool isFemale;
} player_t;

#endif
