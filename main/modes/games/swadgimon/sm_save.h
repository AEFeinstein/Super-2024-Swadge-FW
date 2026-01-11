#ifndef _SM_SAVE_H_
#define _SM_SAVE_H_

#include "sm_constants.h"
#include "sm_monster.h"
#include "sm_player.h"

#define CURRENT_SAVE_FORMAT 0
// Maximum number of bytes per NVS blob. From ESP-IDF documentation
#define NVS_BLOB_MAX_SIZE 1984
// Number of digits to use for box IDs in NVS keys. Includes null character
#define INDEXED_KEY_SUFFIX_LEN 4

static const char sm_namespace_key[] = "sm";
static const char sm_save_fixed_key[] = "save_fixed";
static const char sm_save_monster_box_key_prefix[] = "box_";
static const char sm_save_monster_box_party_key_suffix[] = "party";
static const char sm_save_monster_box_daycare_key_suffix[] = "daycare";
static const char sm_save_trainer_names_key[] = "names_trainer";
static const char sm_save_monster_names_key[] = "names_monster";

struct {
    // Do not change the order or size of members in this struct
    uint8_t saveFormat;
    uint8_t numMonsters;
    uint16_t monsterLength;
} monster_box_header_t;

struct {
    // Do not change the order or size of members in this struct
    uint8_t saveFormat;
    uint8_t nameLength;
    uint16_t numNames;
} names_header_t;

struct {
    uint8_t saveFormat;
    
    player_t player;
    
    monster_instance_t party[PARTY_SIZE];
    monster_instance_party_data_t partyExtraData[PARTY_SIZE];
    
    monster_box_header_t* boxes[NUM_MONSTER_BOXES];
    
    monster_instance_t daycare[NUM_MONSTERS_IN_DAYCARE];
    
    dex_species_t dex[NUM_MONSTERS];
} save_data_fixed_t;

// TODO: copy function prototypes from .c file

#endif
