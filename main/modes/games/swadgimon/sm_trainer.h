#ifndef _SM_TRAINER_H_
#define _SM_TRAINER_H_

#include "sm_constants.h"

// Definitions of trainers that can be battled
struct {
    char* name;
    bool hasCustomMoves;
    monster_trainer_instance_t monsters[PARTY_SIZE];
} trainer_t;

#endif
