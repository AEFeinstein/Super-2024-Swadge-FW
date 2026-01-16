#ifndef _SM_MOVE_H_
#define _SM_MOVE_H_

#include <stdbool.h>
#include <stdint.h>

// Definitions of moves usable by monsters
typedef struct {
    const uint16_t id;
    const char* const name;
    const char* const description;
    const uint8_t effect;
    const uint8_t power;
    const uint8_t type;
    const uint8_t accuracy;
    const uint8_t pp;
    const uint8_t effectChance;
    const uint8_t priority;
    const bool makesContact;
    const bool affectedByProtect;
    const bool affectedByMagicCoat;
    const bool affectedBySnatch;
    const bool usableByMirrorMove;
    // TODO: how will we match move to in-battle and out-of-battle actions?
} move_t;

#endif
