#ifndef _SM_MONSTER_H_
#define _SM_MONSTER_H_

#include <stdbool.h>
#include <stdint.h>
#include "sm_constants.h"

// Definitions of effort values yielded to victor upon defeating monsters of given species, and data in RAM for: a specific monster's inherent "from birth" stats, a specific monster's effort or "training" stats, and number of stat boosting items used on a monster
typedef struct {
    uint8_t hp;
    uint8_t atk;
    uint8_t def;
    uint8_t spAtk;
    uint8_t spDef;
    uint8_t speed;
} monster_ev_yields_t, monster_ivs_t, monster_evs_t, monster_stat_ups_t;

// Definitions of a monster species' base stats, and data in RAM for the final calculated stats from all of a specific monster's various stats (base, IVs, EVs)
typedef struct {
    uint16_t hp;
    uint16_t atk;
    uint16_t def;
    uint16_t spAtk;
    uint16_t spDef;
    uint16_t speed;
} monster_base_stats_t, monster_final_stats_t;

// Data saved to NVS for a specific monster's inherent or "from birth" stats
typedef struct __attribute__((packed)) {
    uint8_t hp:5;
    uint8_t atk:5;
    uint8_t def:5;
    uint8_t spAtk:5;
    uint8_t spDef:5;
    uint8_t speed:5;
} monster_ivs_packed_t;

// Data saved to NVS for number of stat boosting items used on a monster
typedef struct __attribute__((packed)) {
    uint8_t hp:4;
    uint8_t atk:4;
    uint8_t def:4;
    uint8_t spAtk:4;
    uint8_t spDef:4;
    uint8_t speed:4;
} monster_stat_ups_packed_t;

typedef enum {
    TYPE_NONE,
    TYPE_NORMAL,
    TYPE_FIGHTING,
    TYPE_FLYING,
    TYPE_POISON,
    TYPE_GROUND,
    TYPE_ROCK,
    TYPE_BUG,
    TYPE_GHOST,
    TYPE_STEEL,
    TYPE_FIRE,
    TYPE_WATER,
    TYPE_GRASS,
    TYPE_ELECTRIC,
    TYPE_PSYCHIC,
    TYPE_ICE,
    TYPE_DRAGON,
    TYPE_DARK,
    MAX_TYPES_INCL_NULL
} monster_type_t;

// Pattern used to convert a monster's exp value to its level
typedef enum {
    EXP_GROUP_FAST,
    EXP_GROUP_MEDIUM_FAST,
    EXP_GROUP_FLUCTUATING,
    EXP_GROUP_ERRATIC,
    EXP_GROUP_MEDIUM_SLOW,
    EXP_GROUP_SLOW
} exp_group_t;

// How to distribute exp to the player's party monsters after a battle
typedef enum {
    EXP_STRATEGY_LAST, // All exp to the monster that was in the battle when the enemy fainted
    EXP_STRATEGY_PARTICIPATED, // Split exp evenly beteween all monsters that participated in the battle
    EXP_STRATEGY_CATCH_UP_SLOW, // Split exp between all monsters in the party, but give more to lower-level monsters. If all monsters are the same level, give more to monsters with less % progress to the next level. Then, try to keep all monsters at the same %
    EXP_STRATEGY_CATCH_UP_FAST, // All exp to the lowest-level monster. If all monsters are the same level, give all to monsters with less % progress to the next level. Then, try to keep all monsters at the same %
    EXP_STRATEGY_ALL, // Split exp evenly between all monsters in the party
} exp_strategy_t;

// Method required to evolve a specific monster species to a specific other species
typedef enum {
    EVO_NULL,
    EVO_LEVEL,
    EVO_FRIENDSHIP,
    EVO_TRADE,
    EVO_ITEM
} evolution_method_t;

// Struct used for definitions of evolutions a specific monster species to a specific other species
// TODO: propagate const keyword
typedef struct {
    evolution_method_t method;
    uint16_t parameter;
    uint8_t targetMonsterId;
} monster_evolution_t;

// Struct used for definitions of learned moves at specific levels
// TODO: propagate const keyword
typedef struct {
    uint16_t moveId;
    uint8_t level;
} monster_level_up_move_t;

// Definitions of monster species
// TODO: propagate const keyword
typedef struct {
    uint8_t monsterId;
    char* name;
    char* description;
    monster_type_t types[2];
    uint8_t catchRate;
    uint8_t expYield;
    monster_ev_yields_t evYields;
    uint8_t genderRatio; // 0=100% male, 254=100% female, 255=genderless
    monster_base_stats_t baseStats;
    exp_group_t expGroup;
    monster_evolution_t evolutions[3];
    uint16_t* learnableTMs;
    uint16_t* learnableHMs;
    uint16_t* otherValidMoves;
    monster_level_up_move_t* levelUpMoves;
} monster_t;

// Struct used for definitions of battleable trainers' monsters
// TODO: propagate const keyword
typedef struct {
    uint16_t moveIds[4];
    uint8_t monsterId;
    uint8_t level;
} monster_trainer_instance_t;

// Data in RAM for each instance of a monster
typedef struct {
    uint8_t monsterId;
    uint8_t nicknameIdx;
    uint8_t trainerNameIdx;
    uint8_t friendship;
    uint16_t trainerId;
    bool isFemale;
    bool isShiny;
    uint32_t exp;
    uint8_t level;
    monster_ivs_t ivs;
    monster_evs_t evs;
    uint16_t moveIds[NUM_MOVES_PER_MONSTER];
    uint8_t ppUps[NUM_MOVES_PER_MONSTER];
    monster_stat_ups_t statUps;
    // 43 bytes
    // compiler adds 1 byte of padding
} monster_instance_t;

// Data saved to NVS for caught monsters
typedef struct __attribute__((packed)) {
    // bit size comments come after the fields they collate
    uint32_t exp:21;
    uint8_t monsterId:7;
    bool isFemale:1;
    bool isShiny:1;
    // compiler adds 2 bits of padding
    // 4 bytes
    uint8_t nicknameIdx;
    uint8_t trainerNameIdx;
    uint16_t trainerId;
    uint8_t friendship;
    uint8_t level:7;
    // compiler adds 1 bit of padding
    // 6 bytes
    monster_ivs_packed_t ivs; // 4 bytes
    monster_evs_t evs; // 6 bytes
    uint16_t moveIds[NUM_MOVES_PER_MONSTER]; // 8 bytes
    // 18 bytes
    uint8_t ppUps[NUM_MOVES_PER_MONSTER]; // 4 bytes
    monster_stat_ups_packed_t statUps; // 3 bytes
    // 7 bytes
    // TODO: if space allows, add n parity bits to protect 2^n data bits. current n = 9 until we have 513 data bits
} monster_instance_packed_t;

// Current status condition of a monster
typedef enum {
    STATUS_NORMAL,
    STATUS_SLEEP,
    STATUS_FREEZE,
    STATUS_BURN,
    STATUS_PARALYZE,
    STATUS_POISON
} status_condition_t;

// Data in RAM specific to monsters in the player's party
typedef struct {
    uint16_t curHp;
    uint8_t curMovePps[NUM_MOVES_PER_MONSTER];
    status_condition_t statusCondition;
} monster_instance_party_data_t;

// Data saved to NVS specific to monsters in the player's party
typedef struct __attribute__((packed)) {
    uint16_t curHp:10;
    uint8_t curMovePps[NUM_MOVES_PER_MONSTER]; // 24 bits
    status_condition_t statusCondition:3;
    // 37 bits :(
} monster_instance_party_data_packed_t;

// Data in RAM: cached calculated values for a specific monster
typedef struct {
    monster_final_stats_t finalStats;
    uint8_t level;
} monster_instance_cache_data_t;

#endif
