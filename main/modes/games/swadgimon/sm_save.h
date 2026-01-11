#ifndef _SM_SAVE_H_
#define _SM_SAVE_H_

#include "sm_constants.h"
#include "sm_monster.h"
#include "sm_names.h"
#include "sm_player.h"

#define CURRENT_SAVE_FORMAT 0
// Maximum number of bytes per NVS blob. From ESP-IDF documentation
#define NVS_BLOB_MAX_SIZE 1984
// Number of digits to use for box IDs in NVS keys. Includes null character
#define INDEXED_KEY_SUFFIX_LEN 4

typedef struct {
    // Do not change the order or size of members in this struct
    uint8_t saveFormat;
    uint8_t numMonsters;
    uint16_t monsterLength;
} monster_box_header_t;

typedef struct {
    uint32_t trainerId;
    uint16_t numReferences;
} trainer_name_header_t;

typedef struct {
    uint8_t saveFormat;
    
    bool isFemale;
    uint16_t trainerId;
    uint32_t money;
    int64_t timePlayed;
    
    monster_instance_party_data_t partyExtraData[PARTY_SIZE];
} save_data_fixed_t;

typedef struct {
    monster_instance_t party[PARTY_SIZE];
    monster_instance_party_data_t partyExtraData[PARTY_SIZE];
    
    monster_box_header_t* boxes[NUM_MONSTER_BOXES];
    
    monster_instance_t daycare[NUM_MONSTERS_IN_DAYCARE];
    
    dex_species_t dex[NUM_MONSTERS];
} save_data_loaded_t;

// TODO: copy function prototypes from .c file

#endif
