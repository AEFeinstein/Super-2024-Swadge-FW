#ifndef _SM_SAVE_H_
#define _SM_SAVE_H_

#include <stdint.h>

#include "macros.h"

#include "sm_constants.h"
#include "sm_dex.h"
#include "sm_monster.h"
#include "sm_monster_names.h"
#include "sm_names.h"
#include "sm_species_defs.h"

// This comes from partitions.csv, and must be changed in both places simultaneously
// partitions.csv
// emulator/src/components/hdw-nvs.c
#ifndef NVS_PARTITION_SIZE
#define NVS_PARTITION_SIZE   0x6000
#endif

#define CURRENT_SAVE_FORMAT 0
// Maximum number of bytes per NVS blob. From ESP-IDF documentation
//#define NVS_BLOB_MAX_SIZE 1984
#define NVS_BLOB_MAX_SIZE MIN(508000, (uint32_t) (NVS_PARTITION_SIZE * 0.976) - 4000)
// Number of digits to use for box IDs in NVS keys. Includes null character
#define INDEXED_KEY_SUFFIX_LEN 4

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
    monster_box_header_t* boxes[NUM_MONSTER_BOXES];
    
    monster_box_header_t* party;
    monster_box_header_t* daycare;
    
    monster_instance_party_data_t partyExtraData[PARTY_SIZE];
    
    dex_species_t dex[NUM_SPECIES_INCL_NULL];
} save_data_loaded_t;

void initSaveData(save_data_fixed_t* saveData);
void loadSaveData(void);
void saveSaveData(void);
monster_box_header_t* initMonsterBox(uint8_t numMonsters);
monster_box_header_t* initMonsterBoxPacked(uint8_t numMonsters);
void unpackMonster(monster_instance_t* dest, const monster_instance_packed_t* src);
void packMonster(monster_instance_packed_t* dest, const monster_instance_t* src);
bool loadMonsterBox(const char* keySuffix, monster_box_header_t** monsterBoxWithHeader);
bool saveMonsterBox(const char* keySuffix, const monster_box_header_t* monsterBoxWithHeader);
bool loadMonsterBoxByNum(uint8_t boxId, monster_box_header_t** monsterBoxWithHeader);
bool saveMonsterBoxByNum(uint8_t boxId, const monster_box_header_t* monsterBoxWithHeader);
bool checkMonsterBoxIntegrity(monster_box_header_t* monsterBoxWithHeader, names_header_t* trainerNamesWithHeader, names_header_t* monsterNamesWithHeader, char** textOut);
names_header_t* initTrainerNames(void);
bool loadTrainerNames(names_header_t** trainerNamesWithHeader);
bool saveTrainerNames(const names_header_t* trainerNamesWithHeader);
bool loadMonsterNames(names_header_t** monsterNamesWithHeader);
bool saveMonsterNames(const names_header_t* monsterNamesWithHeader);

#endif
