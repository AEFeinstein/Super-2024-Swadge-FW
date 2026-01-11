/*
 * Functions for saving and loading save data for Swadgimon
 */

#include <stdbool.h>
#include <stdio.h>
#include <esp_heap_caps.h>
#include <esp_random.h>
#include "hdw-nvs.h"
#include "sm_save.h"

static const char sm_namespace_key[] = "sm";
static const char sm_save_fixed_key[] = "save_fixed";
static const char sm_save_monster_box_key_prefix[] = "box_";
static const char sm_save_monster_box_party_key_suffix[] = "party";
static const char sm_save_monster_box_daycare_key_suffix[] = "daycare";
static const char sm_save_trainer_names_key[] = "names_trainer";
static const char sm_save_monster_names_key[] = "names_monster";

static const char sm_err_wrong_save_format[] = "The save format is %"PRIu8", but expected %"PRIu8".";
static const char sm_err_wrong_monster_length[] = "Monster length is %"PRIu16" bytes, but expected %"PRIu16".";
static const char sm_err_zero_box_length[] = "The box capacity is 0 monsters.";
static const char sm_err_over_box_length[] = "The box capacity is %"PRIu8" monsters, but expected at most %"PRIu8".";
static const char sm_err_nonzero_monster[] = "The monster at index %"PRIu8" contains data, but no species ID.";
static const char sm_err_over_species_id[] = "The monster at index %"PRIu8" has species ID %"PRIu8", but expected at most %"PRIu8".";
static const char sm_err_over_nickname_idx[] = "The %s at index %"PRIu8" has nickname index %"PRIu16", but expected at most %"PRIu16".";
static const char sm_err_zero_monster_name_length[] = "The %s at index %"PRIu8" with nickname index %"PRIu16" has an empty nickname.";
static const char sm_err_over_monster_name_length[] = "The %s at index %"PRIu8" with nickname index %"PRIu16" has a nickname that's %u characters, but expected at most %"PRIu8".";

// Set default values for a new save
void initSaveData(save_data_fixed_t* saveData) {
    memset(saveData, 0, sizeof(save_data_fixed_t));
    
    saveData->trainerId = (uint16_t) esp_random();
    saveData->money = STARTING_MONEY;
    saveData->timePlayed = 0;
}

// Load the save data from NVS
void loadSaveData() {
    // Declare pointers
    names_header_t* trainerNamesWithHeader;
    names_header_t* monsterNamesWithHeader;
    monster_box_header_t* partyWithHeader;
    monster_box_header_t* daycareWithHeader;
    monster_box_header_t* monsterBoxesWithHeaders[NUM_BOXES];
    
    // Load save data
    bool success = loadTrainerNames(&trainerNamesWithHeader);
    success = loadMonsterNames(&monsterNamesWithHeader);
    success = loadMonsterBox(sm_save_monster_box_party_key_suffix, &partyWithHeader);
    success = loadMonsterBox(sm_save_monster_box_daycare_key_suffix, &daycareWithHeader);
    for(int i = 0; i < NUM_BOXES; i++) {
        success = loadMonsterBoxByNum(i, monsterBoxesWithHeaders[i])
    }
    
    // Derive pointers to data after headers
    char (*trainerNames)[] = (char*)[] &trainerNamesWithHeader[1];
    char (*monsterNames)[] = (char*)[] &monsterNamesWithHeader[1];
    monster_instance_t (*party)[] = (monster_instance_t*)[] &partyWithHeader[1];
    monster_instance_t (*daycare)[] = (monster_instance_t*)[] &daycareWithHeader[1];
    
    // TODO: handle fixed save data
    
    // TODO: put all these pointers somewhere with a scope beyond this function
}

// Save the save data to NVS
void saveSaveData() {
    // TODO: get all these pointers from somewhere
    
    bool success = saveTrainerNames(trainerNamesWithHeader);
    success = saveMonsterNames(monsterNamesWithHeader);
    
    success = saveMonsterBox(sm_save_monster_box_party_key_suffix, partyWithHeader);
    success = saveMonsterBox(sm_save_monster_box_daycare_key_suffix, daycareWithHeader);
    for(int i = 0; i < NUM_BOXES; i++) {
        success = saveMonsterBoxByNum(i, monsterBoxesWithHeaders[i])
    }
}

// Allocate memory for a monster box structure and set its header fields
static monster_box_header_t* initMonsterBoxVarSize(const uint16_t monsterLength, const uint8_t numMonsters, bool doBlobSizeCheck) {
    size_t blobLength = sizeof(monster_box_header_t) + monsterLength * numMonsters;
    
    if(doBlobSizeCheck) {
        // Bounds check
        // This array needs to fit in an NVS blob
        if(monsterLength * numMonsters > NVS_BLOB_MAX_SIZE)
            return NULL;
    }
    
    // Allocate required memory
    monster_box_header_t* monsterBoxWithHeader = (monster_box_header_t*) heap_caps_calloc(blobLength, 1, MALLOC_CAP_SPIRAM);
    
    // Check whether memory was allocated
    if(monsterBoxWithHeader == NULL) {
        return NULL;
    }
    
    // Set header fields
    monsterBoxWithHeader->saveFormat = CURRENT_SAVE_FORMAT;
    monsterBoxWithHeader->monsterLength = monsterLength;
    monsterBoxWithHeader->numMonsters = numMonsters;
    
    return monsterBoxWithHeader;
}

// Allocate memory for an unpacked monster box structure and set its header fields
monster_box_header_t* initMonsterBox(const uint8_t numMonsters) {
    return initMonsterBoxVarSize(sizeof(monster_instance_t), numMonsters, false);
}

// Allocate memory for a packed monster box structure and set its header fields
monster_box_header_t* initMonsterBoxPacked(const uint8_t numMonsters) {
    return initMonsterBoxVarSize(sizeof(monster_instance_packed_t), numMonsters, true);
}

void unpackMonster(monster_instance_t* dest, monster_instance_packed_t* src) {
    dest->nicknameIdx = src->nicknameIdx;
    dest->trainerNameIdx = src->trainerNameIdx;
    dest->trainerId = src->trainerId;
    dest->friendship = src->friendship;
    dest->monsterId = src->monsterId;
    dest->isFemale = src->isFemale;
    dest->isShiny = src->isShiny;
    dest->exp = src->exp;
    
    dest->ivs.hp = src->ivs.hp;
    dest->ivs.atk = src->ivs.atk;
    dest->ivs.def = src->ivs.def;
    dest->ivs.spAtk = src->ivs.spAtk;
    dest->ivs.spDef = src->ivs.spDef;
    dest->ivs.speed = src->ivs.speed;
    
    dest->evs.hp = src->evs.hp;
    dest->evs.atk = src->evs.atk;
    dest->evs.def = src->evs.def;
    dest->evs.spAtk = src->evs.spAtk;
    dest->evs.spDef = src->evs.spDef;
    dest->evs.speed = src->evs.speed;
    
    for(uint8_t i = 0; i < NUM_MOVES_PER_MONSTER; i++){
        dest->moveIds[i] = src->moveIds[i];
        dest->ppUps[i] = src->ppUps[i];
    }
    
    dest->statUps.hp = src->statUps.hp;
    dest->statUps.atk = src->statUps.atk;
    dest->statUps.def = src->statUps.def;
    dest->statUps.spAtk = src->statUps.spAtk;
    dest->statUps.spDef = src->statUps.spDef;
    dest->statUps.speed = src->statUps.speed;
}

void packMonster(monster_instance_packed_t* dest, monster_instance_t* src) {
    dest->nicknameIdx = src->nicknameIdx;
    dest->trainerNameIdx = src->trainerNameIdx;
    dest->trainerId = src->trainerId;
    dest->friendship = src->friendship;
    dest->monsterId = src->monsterId;
    dest->isFemale = src->isFemale;
    dest->isShiny = src->isShiny;
    dest->exp = src->exp;
    
    dest->ivs.hp = src->ivs.hp;
    dest->ivs.atk = src->ivs.atk;
    dest->ivs.def = src->ivs.def;
    dest->ivs.spAtk = src->ivs.spAtk;
    dest->ivs.spDef = src->ivs.spDef;
    dest->ivs.speed = src->ivs.speed;
    
    dest->evs.hp = src->evs.hp;
    dest->evs.atk = src->evs.atk;
    dest->evs.def = src->evs.def;
    dest->evs.spAtk = src->evs.spAtk;
    dest->evs.spDef = src->evs.spDef;
    dest->evs.speed = src->evs.speed;
    
    for(uint8_t i = 0; i < NUM_MOVES_PER_MONSTER; i++){
        dest->moveIds[i] = src->moveIds[i];
        dest->ppUps[i] = src->ppUps[i];
    }
    
    dest->statUps.hp = src->statUps.hp;
    dest->statUps.atk = src->statUps.atk;
    dest->statUps.def = src->statUps.def;
    dest->statUps.spAtk = src->statUps.spAtk;
    dest->statUps.spDef = src->statUps.spDef;
    dest->statUps.speed = src->statUps.speed;
}

// Load a monster box structure from NVS with a given string key suffix
bool loadMonsterBox(char* keySuffix, monster_box_header_t** monsterBoxWithHeader) {
    // Combine the key prefix and suffix
    char[NVS_KEY_NAME_MAX_SIZE] key;
    size_t snprintfRetVal = snprintf(key, NVS_KEY_NAME_MAX_SIZE, "%s%s", sm_save_monster_box_key_prefix, keySuffix);
    // Check whether any characters were truncated
    if(snprintfRetVal < 0 || snprintfRetVal >= NVS_KEY_NAME_MAX_SIZE) {
        return false;
    }
    
    size_t blobLength;
    // Get the length of the blob from NVS
    if(!readNamespaceNvsBlob(sm_namespace_key, key, NULL, &blobLength))
        return false;
    
    // Bounds check 1
    // The blob needs to be large enough to at least store the header information
    if(blobLength < sizeof(monster_box_header_t))
        return false;
    
    // Read the blob from NVS
    monster_box_header_t* monsterBoxWithHeaderPacked = (monster_box_header_t*) heap_caps_calloc(blobLength, 1, MALLOC_CAP_SPIRAM);
    if(!readNamespaceNvsBlob(sm_namespace_key, key, monsterBoxWithHeaderPacked, &blobLength)) {
        free(monsterBoxWithHeaderPacked);
        return false;
    }
    
    // Integrity check 1
    // The save format needs to be one we know how to read
    // We currently only know how to read a single format
    if(monsterBoxWithHeaderPacked->saveFormat != CURRENT_SAVE_FORMAT ||
       monsterBoxWithHeaderPacked->monsterLength != sizeof(monster_instance_packed_t) ||
       monsterBoxWithHeaderPacked->numMonsters == 0 ||
       monsterBoxWithHeaderPacked->numMonsters > NUM_MONSTERS_PER_BOX_X * NUM_MONSTERS_PER_BOX_Y ||
       // Bounds check 2
       // The rest of the blob after the header needs to have the exact number of bytes expected by multiplying the number of bytes per name with the number of names
       blobLength - sizeof(monster_box_header_t) != monsterBoxWithHeaderPacked->numMonsters * monsterBoxWithHeaderPacked->monsterLength) {
        free(monsterBoxWithHeaderPacked);
        return false;
    }
    
    // Initialize an unpacked monster box structure
    *monsterBoxWithHeader = initMonsterBox(monsterBoxWithHeaderPacked->numMonsters);
    if(monsterBoxWithHeader == NULL) {
        free(monsterBoxWithHeaderPacked);
        return false;
    }
    
    // Derive pointers to data after headers
    monster_instance_t (monsterBoxPacked)[] = (monster_instance_packed_t)[] &monsterBoxWithHeaderPacked[1];
    monster_instance_t (monsterBox)[] = (monster_instance_t)[] &(*monsterBoxWithHeader)[1];
    
    // Copy the monsters from the packed structure into the unpacked structure
    int16_t firstNonEmptyMonsterIdx = -1;
    monster_instace_packed_t emptyMonsterPacked = {0};
    for(uint8_t i = 0; i < monsterBoxWithHeader->numMonsters; i++) {
        unpackMonster(&monsterBox[i], &monsterBoxPacked[i]);
        if(firstNonEmptyMonsterIdx > -1 && memcmp(&emptyMonsterPacked, &monsterBoxPacked) == 0) {
            firstNonEmptyMonsterIdx = i;
        }
    }
    
    // Integrity check 2
    // Repack the first non-empty monster in the box and make sure it matches the original data from NVS
    if(firstNonEmptyMonsterIdx > -1) {
        monster_instance_packed_t packTest;
        packMonster(&packTest, &monsterBox[firstNonEmptyMonsterIdx]);
        if(memcmp(&packTest, &monsterBoxPacked[firstNonEmptyMonsterIdx], sizeof(monster_instance_packed_t)) != 0) {
            free(monsterBoxWithHeaderPacked);
            free(monsterBoxWithHeader);
            return false;
        }
    }
    
    // Free the packed structure in memory
    free(monsterBoxWithHeaderPacked);
    
    return true;
}

// Save a monster box structure to NVS with a given string key suffix
bool saveMonsterBox(char* keySuffix, monster_box_header_t* monsterBoxWithHeader) {
    // Bounds check 1
    // The number of monsters in this box must be between 1 and the current X*Y size, inclusive
    if(monsterBoxWithHeaderPacked->numMonsters == 0 ||
       monsterBoxWithHeaderPacked->numMonsters > NUM_MONSTERS_PER_BOX_X * NUM_MONSTERS_PER_BOX_Y) {
        return false;
    }
    
    // Combine the key prefix and suffix
    char[NVS_KEY_NAME_MAX_SIZE] key;
    size_t snprintfRetVal = snprintf(key, NVS_KEY_NAME_MAX_SIZE, "%s%s", sm_save_monster_box_key_prefix, keySuffix);
    // Check whether any characters were truncated
    if(snprintfRetVal < 0 || snprintfRetVal >= NVS_KEY_NAME_MAX_SIZE) {
        return false;
    }
    
    // Initialize an unpacked monster box structure
    monster_box_header_t* monsterBoxWithHeaderPacked = initMonsterBoxPacked(monsterBoxWithHeader->numMonsters);
    if(monsterBoxWithHeaderPacked == NULL) {
        return false;
    }
    
    // Integrity check 2
    // The save format needs to be one we know how to write
    // We currently only know how to write a single format
    if(monsterBoxWithHeaderPacked->saveFormat != CURRENT_SAVE_FORMAT ||
       monsterBoxWithHeaderPacked->monsterLength != sizeof(monster_instance_packed_t)) {
        free(monsterBoxWithHeaderPacked);
        return false;
    }
    
    // Derive pointers to data after headers
    monster_instance_t (monsterBoxPacked)[] = (monster_instance_packed_t)[] &monsterBoxWithHeaderPacked[1];
    monster_instance_t (monsterBox)[] = (monster_instance_t)[] &monsterBoxWithHeader[1];
    
    // Copy the monsters from the packed structure into the unpacked structure
    int16_t firstNonEmptyMonsterIdx = -1;
    monster_instace_t emptyMonster = {0};
    for(uint8_t i = 0; i < monsterBoxWithHeader->numMonsters; i++) {
        packMonster(&monsterBoxPacked[i], &monsterBox[i]);
        if(firstNonEmptyMonsterIdx > -1 && memcmp(&emptyMonster, &monsterBox) == 0) {
            firstNonEmptyMonsterIdx = i;
        }
    }
    
    // Integrity check 3
    // Unpack the first non-empty monster in the freshly-packed box and make sure it matches the original data from memory
    if(firstNonEmptyMonsterIdx > -1) {
        monster_instance_t unpackTest;
        unpackMonster(&unpackTest, &monsterBoxPacked[firstNonEmptyMonsterIdx]);
        if(memcmp(&unpackTest, &monsterBox[firstNonEmptyMonsterIdx], sizeof(monster_instance_t)) != 0) {
            free(monsterBoxWithHeaderPacked);
            return false;
        }
    }
    
    // Calculate the length of the blob to write
    size_t blobLength = sizeof(monster_box_header_t)) + monsterBoxWithHeaderPacked->monsterLength * monsterBoxWithHeaderPacked->numMonsters;
    
    // Write the blob to NVS
    if(!writeNamespaceNvsBlob(sm_namespace_key, key, monsterBoxWithHeaderPacked, blobLength)) {
        free(monsterBoxWithHeaderPacked);
        return false;
    }
    
    // Free the packed structure in memory
    free(monsterBoxWithHeaderPacked);
    
    return true;
}

bool loadMonsterBoxByNum(uint8_t boxId, monster_box_header_t** monsterBoxWithHeader) {
    // Convert the box ID into a string
    char[INDEXED_KEY_SUFFIX_LEN] keySuffix;
    size_t snprintfRetVal = snprintf(keySuffix, INDEXED_KEY_SUFFIX_LEN, "%0*" PRIu8, INDEXED_KEY_SUFFIX_LEN - 1, boxId)
    // Check whether any characters were truncated
    if(snprintfRetVal < 0 || snprintfRetVal >= INDEXED_KEY_SUFFIX_LEN) {
        return false;
    }
    
    // Load the box data
    return loadMonsterBox(keySuffix, monsterBoxWithHeader);
}

bool saveMonsterBoxByNum(uint8_t boxId, monster_box_header_t* monsterBoxWithHeader) {
    // Convert the box ID into a string
    char[INDEXED_KEY_SUFFIX_LEN] keySuffix;
    size_t snprintfRetVal = snprintf(keySuffix, INDEXED_KEY_SUFFIX_LEN, "%0*" PRIu8, INDEXED_KEY_SUFFIX_LEN - 1, boxId)
    // Check whether any characters were truncated
    if(snprintfRetVal < 0 || snprintfRetVal >= INDEXED_KEY_SUFFIX_LEN) {
        return false;
    }
    
    // Load the box data
    return saveMonsterBox(keySuffix, monsterBoxWithHeader);
}

// Can only check an unpacked monster box
bool checkMonsterBoxIntegrity(monster_box_header_t* monsterBoxWithHeader, names_header_t* trainerNamesWithHeader, names_header_t* monsterNamesWithHeader, char** textOut) {
    // Allocate memory for the error string
    if(textOut != NULL) {
        if(*textOut != NULL) {
            free(*textOut);
        }
        *textOut = (char*) heap_caps_calloc(MAX_ERROR_LEN, 1, MALLOC_CAP_SPIRAM);
    }
    
    // The save format needs to be one we know how to read and write
    // We currently only know how to read and write one format
    if(monsterBoxWithHeader->saveFormat != CURRENT_SAVE_FORMAT) {
        if(textOut && *textOut) {
            snprintf(*textOut, MAX_ERROR_LEN, sm_err_wrong_save_format, monsterBoxWithHeader->saveFormat, CURRENT_SAVE_FORMAT);
        }
        return false;
    }
    
    // The monster length must match the length of the unpacked monster format
    if(monsterBoxWithHeader->monsterLength != sizeof(monster_instance_t)) {
        if(textOut && *textOut) {
            snprintf(*textOut, MAX_ERROR_LEN, sm_err_wrong_monster_length, monsterBoxWithHeader->monsterLength, sizeof(monster_instance_t));
        }
        return false;
    }
    
    // The box must have capacity for more than 0 monsters
    if(monsterBoxWithHeader->numMonsters == 0) {
        if(textOut && *textOut) {
            snprintf(*textOut, MAX_ERROR_LEN, sm_err_zero_box_length);
        }
        return false;
    }
    
    // The box must have capacity for at most X * Y monsters
    if(monsterBoxWithHeader->numMonsters > NUM_MONSTERS_PER_BOX_X * NUM_MONSTERS_PER_BOX_Y) {
       if(textOut && *textOut) {
            snprintf(*textOut, MAX_ERROR_LEN, sm_err_over_box_length, monsterBoxWithHeader->numMonsters, NUM_MONSTERS_PER_BOX_X * NUM_MONSTERS_PER_BOX_Y);
        }
        return false; 
    }
    
    TODO: check trainer and monster name header contents
    
    // Derive pointers to data after headers
    monster_instance_t (monsterBox)[] = (monster_instance_t)[] &monsterBoxWithHeader[1];
    char (*trainerNames)[] = (char*)[] &trainerNamesWithHeader[1];
    char (*monsterNames)[] = (char*)[] &monsterNamesWithHeader[1];
    
    monster_instace_t emptyMonster = {0};
    
    for(uint8_t i = 0; i < monsterBoxWithHeader->numMonsters; i++) {
        // If the monster's species ID is 0, all other fields must be 0
        if(monsterBox[i].monsterId == 0) {
            if(strcmp(monsterBox[i], emptyMonster, sizeof(monster_instance_t)) != 0) {
                if(textOut && *textOut) {
                    snprintf(*textOut, MAX_ERROR_LEN, sm_err_nonzero_monster, i);
                }
                return false;
            }
            
            // This monster is empty, so there's nothing else to check
            continue;
        }
        
        // The species ID must be valid
        if(monsterBox[i].monsterId >= NUM_SPECIES_INCL_NULL) {
            if(strcmp(monsterBox[i], emptyMonster, sizeof(monster_instance_t)) != 0) {
                if(textOut && *textOut) {
                    snprintf(*textOut, MAX_ERROR_LEN, sm_err_over_species_id, i, monsterBox[i].monsterId, NUM_SPECIES_INCL_NULL - 1);
                }
                return false;
            }
        }
        
        // If the monster has a nickname, it must exist in the monster names structure
        if(monsterBox[i].nicknameIdx > 0) {
            // The nickname index must be within the bounds of the monster names structure
            if(monsterBox[i].nicknameIdx >= monsterNamesWithHeader->numNames) {
                if(textOut && *textOut) {
                    snprintf(*textOut, MAX_ERROR_LEN, sm_err_over_nickname_idx, speciesDefs[monsterBox[i].monsterId].name, i, monsterBox[i].nicknameIdx, monsterNamesWithHeader->numNames);
                }
                return false;
            }
            
            // The monster name in the structure must have a length greater than 0
            size_t nameLength = strlen(monsterNames[monsterBox[i].nicknameIdx]);
            if(nameLength == 0) {
                if(textOut && *textOut) {
                    snprintf(*textOut, MAX_ERROR_LEN, sm_err_zero_monster_name_length, speciesDefs[monsterBox[i].monsterId].name, i, monsterBox[i].nicknameIdx);
                }
                return false;
            }
            
            // The monster name in the structure must have a length less than or equal to the maximum length allowed in the structure
            if(nameLength > monsterNamesWithHeader->nameLength) {
                if(textOut && *textOut) {
                    snprintf(*textOut, MAX_ERROR_LEN, sm_err_over_monster_name_length, speciesDefs[monsterBox[i].monsterId].name, i, monsterBox[i].nicknameIdx, monsterNamesWithHeader->numNames);
                }
                return false;
            }
            
            // TODO: count references to each monster name, and make sure all of them have 0 or 1 references, except index 0 which is allowed to have any number of references
            
            
        }
        strlen(trainerNames[i])
        
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
    }
}

// Allocate memory for a names structure and set its header fields
static names_header_t* initNames(const uint8_t nameLength, const uint16_t numNames) {
    size_t blobLength = sizeof(names_header_t) + nameLength * numNames;
    
    // Bounds check
    // This array needs to fit in an NVS blob
    if(blobLength > NVS_BLOB_MAX_SIZE)
        return NULL;
    
    // Allocate minimum required memory
    names_header_t* namesWithHeader = (names_header_t*) heap_caps_calloc(blobLength, 1, MALLOC_CAP_SPIRAM);
    
    // Set header fields
    namesWithHeader->saveFormat = CURRENT_SAVE_FORMAT;
    namesWithHeader->nameLength = nameLength;
    namesWithHeader->numNames = numNames;
}

// Load a names structure from NVS with a given string key
static bool loadNames(const char* key, names_header_t** namesWithHeader, uint8_t maxNameLength) {
    size_t blobLength;
    // Get the length of the blob from NVS
    if(!readNamespaceNvsBlob(sm_namespace_key, key, NULL, &blobLength))
        return false;
    
    // Bounds check 1
    // The blob needs to be large enough to at least store the header information
    if(blobLength < sizeof(names_header_t))
        return false;
    
    // Read the blob from NVS
    *namesWithHeader = (names_header_t*) heap_caps_calloc(blobLength, 1, MALLOC_CAP_SPIRAM);
    if(!readNamespaceNvsBlob(sm_namespace_key, key, *namesWithHeader, &blobLength))
        return false;
    
    // Integrity check
    // The save format needs to be one we know how to read
    // We currently only know how to read a single format
    if(*namesWithHeader->saveFormat != CURRENT_SAVE_FORMAT ||
       *namesWithHeader->nameLength > maxNameLength)
        return false;
    
    // Bounds check 2
    // The rest of the blob after the header needs to have the exact number of bytes expected by multiplying the number of bytes per name with the number of names
    size_t dataLength = blobLength - sizeof(names_header_t);
    if(dataLength != *namesWithHeader->nameLength * *namesWithHeader->numNames)
        return false;
    
    return true;
}

// Save a names structure to NVS with a given string key
static bool saveNames(const char* key, names_header_t* namesWithHeader) {
    return writeNamespaceNvsBlob(sm_namespace_key, key, namesWithHeader, sizeof(names_header_t) + namesWithHeader->nameLength * namesWithHeader->numNames);
}

// Initialize the trainer names structure
names_header_t* initTrainerNames() {
    return initNames(MAX_TRAINER_NAME_LEN, MIN_TRAINER_NAMES);
}

// Load the trainer names structure from NVS
bool loadTrainerNames(names_header_t** trainerNamesWithHeader) {
    return loadNames(sm_save_trainer_names_key, trainerNamesWithHeader, MAX_TRAINER_NAME_LEN);
}

// Save the trainer names structure to NVS
bool saveTrainerNames(names_header_t* trainerNamesWithHeader) {
    return saveNames(sm_save_trainer_names_key, trainerNamesWithHeader);
}

// Initialize the monster names structure
names_header_t* initMonsterNames() {
    return initNames(MAX_MONSTER_NAME_LEN, 0);
}

// Load the monster names structure from NVS
bool loadMonsterNames(char** monsterNames) {
    return loadNames(sm_save_monster_names_key, monsterNames, MAX_MONSTER_NAME_LEN);
}

// Save the monster names structure to NVS
bool saveMonsterNames(names_header_t* monsterNamesWithHeader) {
    return saveNames(sm_save_monster_names_key, monsterNamesWithHeader);
}
