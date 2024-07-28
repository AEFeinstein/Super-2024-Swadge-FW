#ifndef _SM_MOVE_H_
#define _SM_MOVE_H_

// Definitions of moves usable by monsters
struct {
    uint16_t id;
    char* name;
    char* description;
    uint8_t effect;
    uint8_t power;
    uint8_t type;
    uint8_t accuracy;
    uint8_t pp;
    uint8_t effectChance;
    uint8_t priority;
    bool makesContact;
    bool affectedByProtect;
    bool affectedByMagicCoat;
    bool affectedBySnatch;
    bool usableByMirrorMove;
    // TODO: how will we match move to in-battle and out-of-battle actions?
} move_t;

#endif
