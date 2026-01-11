#ifndef _SM_SAVE_H_
#define _SM_SAVE_H_

#include <stdint.h>
#include "sm_constants.h"
#include "sm_dex.h"
#include "sm_monster.h"
#include "sm_names.h"
#include "sm_species_defs.h"

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
    
    dex_species_t dex[NUM_SPECIES_INCL_NULL];
} save_data_loaded_t;

void initSaveData(save_data_fixed_t* saveData);
void loadSaveData();
void saveSaveData();
static monster_box_header_t* initMonsterBoxVarSize(const uint16_t monsterLength, const uint8_t numMonsters, bool doBlobSizeCheck);
monster_box_header_t* initMonsterBox(const uint8_t numMonsters);
monster_box_header_t* initMonsterBoxPacked(const uint8_t numMonsters);
void unpackMonster(monster_instance_t* dest, monster_instance_packed_t* src);
void packMonster(monster_instance_packed_t* dest, monster_instance_t* src);
bool loadMonsterBox(char* keySuffix, monster_box_header_t** monsterBoxWithHeader);
bool saveMonsterBox(char* keySuffix, monster_box_header_t* monsterBoxWithHeader);
bool loadMonsterBoxByNum(uint8_t boxId, monster_box_header_t** monsterBoxWithHeader);
bool saveMonsterBoxByNum(uint8_t boxId, monster_box_header_t* monsterBoxWithHeader);
bool checkMonsterBoxIntegrity(monster_box_header_t* monsterBoxWithHeader, names_header_t* trainerNamesWithHeader, names_header_t* monsterNamesWithHeader, char** textOut);
static names_header_t* initNames(const uint8_t nameLength, const uint16_t numNames);
static bool loadNames(const char* key, names_header_t** namesWithHeader, uint8_t maxNameLength);
static bool saveNames(const char* key, names_header_t* namesWithHeader);
names_header_t* initTrainerNames();
bool loadTrainerNames(names_header_t** trainerNamesWithHeader);
bool saveTrainerNames(names_header_t* trainerNamesWithHeader);
names_header_t* initMonsterNames();
bool loadMonsterNames(names_header_t** monsterNames);
bool saveMonsterNames(names_header_t* monsterNamesWithHeader);

#endif
