#ifndef _SM_MONSTER_H_
#define _SM_MONSTER_H_

// Definitions of effort values yielded to victor upon defeating monsters of given species, and data in RAM for: a specific monster's inherent "from birth" stats, a specific monster's effort or "training" stats, and number of stat boosting items used on a monster
struct {
    uint8_t hp;
    uint8_t atk;
    uint8_t def;
    uint8_t spAtk;
    uint8_t spDef;
    uint8_t speed;
} monster_ev_yields_t, monster_ivs_t, monster_evs_t, monster_stat_ups_t;

// Definitions of a monster species' base stats, and data in RAM for the final calculated stats from all of a specific monster's various stats (base, IVs, EVs)
struct {
    uint16_t hp;
    uint16_t atk;
    uint16_t def;
    uint16_t spAtk;
    uint16_t spDef;
    uint16_t speed;
} monster_base_stats_t, monster_final_stats_t;

// Data saved to NVS for a specific monster's inherent or "from birth" stats
struct __attribute__((packed)) {
    uint8_t hp:5;
    uint8_t atk:5;
    uint8_t def:5;
    uint8_t spAtk:5;
    uint8_t spDef:5;
    uint8_t speed:5;
} monster_ivs_packed_t;

// Data saved to NVS for a specific monster's effort or "training" stats
struct __attribute__((packed)) {
    uint8_t hp;
    uint8_t atk;
    uint8_t def;
    uint8_t spAtk;
    uint8_t spDef;
    uint8_t speed;
} monster_evs_packed_t;

// Data saved to NVS for number of stat boosting items used on a monster
struct __attribute__((packed)) {
    uint8_t hp:4;
    uint8_t atk:4;
    uint8_t def:4;
    uint8_t spAtk:4;
    uint8_t spDef:4;
    uint8_t speed:4;
} monster_stat_ups_packed_t;

// Pattern used to convert a monster's exp value to its level
enum {
    EXP_GROUP_MEDIUM_FAST,
    EXP_GROUP_ERRATIC,
    EXP_GROUP_FLUCTUATING,
    EXP_GROUP_MEDIUM_SLOW,
    EXP_GROUP_FAST,
    EXP_GROUP_SLOW
} exp_group_t;

// Method required to evolve a specific monster species to a specific other species
enum {
    EVO_NULL,
    EVO_LEVEL,
    EVO_FRIENDSHIP,
    EVO_TRADE,
    EVO_ITEM
} evolution_method_t;

// Struct used for definitions of evolutions a specific monster species to a specific other species
// TODO: propagate const keyword
struct {
    evolution_method_t method;
    uint16_t parameter;
    uint8_t targetMonsterId;
} monster_evolution_t;

// Struct used for definitions of learned moves at specific levels
// TODO: propagate const keyword
struct {
    uint8_t level;
    uint16_t moveId;
} monster_level_up_move_t;

// Definitions of monster species
// TODO: propagate const keyword
struct {
    uint8_t monsterId;
    char* name;
    char* description;
    uint8_t types[2];
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
struct {
    uint8_t monsterId;
    uint8_t level;
    uint16_t moveIds[4];
} monster_trainer_instance_t;

// Data in RAM for each instance of a monster
struct {
    uint8_t nicknameIdx;
    uint8_t trainerNameIdx;
    uint16_t trainerId;
    uint8_t friendship;
    uint8_t monsterId;
    bool isFemale;
    bool isShiny;
    uint32_t exp;
    monster_ivs_t ivs;
    monster_evs_t evs;
    uint16_t moveIds[4];
    uint8_t ppUps[4];
    monster_stat_ups_t statUps;
} monster_instance_t;

// Data saved to NVS for caught monsters
struct __attribute__((packed)) {
    // bit size comments come after the fields they collate
    uint8_t nicknameIdx;
    uint8_t trainerNameIdx;
    uint16_t trainerId;
    uint8_t friendship;
    // 5 bytes
    uint8_t monsterId:7;
    bool isFemale:1;
    // 8 bits
    bool isShiny:1;
    uint32_t exp:21;
    monster_ivs_packed_t ivs:30;
    monster_evs_packed_t evs:48;
    uint16_t moveIds:9[4]; // 36 bits
    // 17 bytes
    uint8_t ppUps:2[4];
    monster_stat_ups_packed_t statUps:24;
    // 32 bits
    // TODO: if space allows, add n parity bits to protect 2^n data bits. current n = 9 until we have 513 data bits
} monster_instance_packed_t;

// Data in RAM specific to monsters in the player's party
struct {
    uint16_t curHp;
    uint8_t curMovePps[4];
    uint8_t statusCondition;
} monster_instance_party_data_t;

// Data saved to NVS specific to monsters in the player's party
struct __attribute__((packed)) {
    uint16_t curHp:10;
    uint8_t curMovePps:6[4]; // 24 bits
    uint8_t statusCondition:3;
    // 37 bits :(
} monster_instance_party_data_packed_t;

// Data in RAM: cached calculated values for a specific monster
struct {
    monster_final_stats_t finalStats;
    uint8_t level;
} monster_instance_cache_data_t;

#endif
