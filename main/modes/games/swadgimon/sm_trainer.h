#ifndef _SM_TRAINER_H_
#define _SM_TRAINER_H_

#include <stdbool.h>

#include "sm_constants.h"
#include "sm_monster.h"

// Definitions of trainers that can be battled
struct {
    const char* name;
    const bool hasCustomMoves;
    const monster_trainer_instance_t const monsters[PARTY_SIZE];
} trainer_t;

#endif
