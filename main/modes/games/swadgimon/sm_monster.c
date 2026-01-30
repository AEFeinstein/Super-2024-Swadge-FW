#include <assert.h>
#include <math.h>
#include <string.h>

#include <macros.h>
#include "esp_random.h"

#include "sm_monster.h"
#include "sm_monster_names.h"
#include "sm_species_defs.h"



// When the player loses a battle, this is multiplied by the level of the player's highest-level party monster to determine how much money they lose
static uint16_t numBadgesToBasePayout[] = {8, 24, 48, 80, 120};



// Return a pointer to the monster at the given index, or NULL if the index is past the bounds of the array
// Runs in O(1)
monster_instance_t* monsterBoxGet(const monster_box_header_t* monsterBoxWithHeader, uint8_t idx) {
    assert(monsterBoxWithHeader);
    
    if(idx >= monsterBoxWithHeader->numMonsters) {
        return NULL;
    }
    
    return &((monster_instance_t*) &monsterBoxWithHeader[1])[idx * monsterBoxWithHeader->monsterLength];
}

void generateWildMonsterBySpecies(monster_instance_t* monsterOut, monster_instance_party_data_t* partyDataOut, monster_final_stats_t* finalStatsOut, uint8_t speciesId, uint8_t levelMin, uint8_t levelMax) {
    const monster_t* species = &speciesDefs[speciesId];

    monsterOut->monsterId = species->monsterId;
    monsterOut->nicknameIdx = MONSTER_NAME_IDX_NONE;
    monsterOut->trainerNameIdx = TRAINER_NAME_IDX_NONE;
    monsterOut->friendship = 0;
    monsterOut->isFemale = (esp_random() % GENDER_RATIO_GENDERLESS) < species->genderRatio;
    monsterOut->isShiny = (esp_random() % SHINY_DIVISOR) == 0;
    monsterOut->level = esp_random() % (levelMax - levelMin) + 1; // TODO: use a normal distribution?
    monsterOut->exp = getTotalExpToLevel(monsterOut->level, species->expGroup);
    
    monsterOut->ivs.hp = esp_random() % (MAX_IV + 1);
    monsterOut->ivs.atk = esp_random() % (MAX_IV + 1);
    monsterOut->ivs.def = esp_random() % (MAX_IV + 1);
    monsterOut->ivs.spAtk = esp_random() % (MAX_IV + 1);
    monsterOut->ivs.spDef = esp_random() % (MAX_IV + 1);
    monsterOut->ivs.speed = esp_random() % (MAX_IV + 1);
    
    monsterOut->evs.hp = 0;
    monsterOut->evs.atk = 0;
    monsterOut->evs.def = 0;
    monsterOut->evs.spAtk = 0;
    monsterOut->evs.spDef = 0;
    monsterOut->evs.speed = 0;
    
    for(int i = 0; i < NUM_MOVES_PER_MONSTER; i++) {
        // TODO: generate moveset from species->levelUpMoves
        monsterOut->moveIds[i] = 0;
        monsterOut->ppUps[i] = 0;
        partyDataOut->curMovePps[i] = 0;
    }
    
    monsterOut->statUps.hp = 0;
    monsterOut->statUps.atk = 0;
    monsterOut->statUps.def = 0;
    monsterOut->statUps.spAtk = 0;
    monsterOut->statUps.spDef = 0;
    monsterOut->statUps.speed = 0;
    
    getFinalStats(finalStatsOut, monsterOut);
    
    partyDataOut->curHp = finalStatsOut->hp;
    partyDataOut->statusCondition = STATUS_NORMAL;
}

const char* getDisplayName(const monster_instance_t* monster, const names_header_t* monsterNamesWithHeader) {
    if(monster->nicknameIdx > 0) {
        return monsterNamesGet(monsterNamesWithHeader, monster->nicknameIdx);
    }
    else {
        return speciesDefs[monster->monsterId].name;
    }
}

// Calculate all effective stats from their individual stat components
void getFinalStats(monster_final_stats_t* finalStats, const monster_instance_t* monster) {
    const monster_base_stats_t* baseStats = &speciesDefs[monster->monsterId].baseStats;
    
    finalStats->hp = getFinalStatHp(baseStats->hp, monster->ivs.hp, monster->evs.hp, monster->statUps.hp, monster->level);
    finalStats->atk = getFinalStatNonHp(baseStats->atk, monster->ivs.atk, monster->evs.atk, monster->statUps.atk, monster->level);
    finalStats->def = getFinalStatNonHp(baseStats->def, monster->ivs.def, monster->evs.def, monster->statUps.def, monster->level);
    finalStats->spAtk = getFinalStatNonHp(baseStats->spAtk, monster->ivs.spAtk, monster->evs.spAtk, monster->statUps.spAtk, monster->level);
    finalStats->spDef = getFinalStatNonHp(baseStats->spDef, monster->ivs.spDef, monster->evs.spDef, monster->statUps.spDef, monster->level);
    finalStats->speed = getFinalStatNonHp(baseStats->speed, monster->ivs.speed, monster->evs.speed, monster->statUps.speed, monster->level);
}

// Calculate effective HP stat from its individual stat components
uint16_t getFinalStatHp(uint16_t baseStat, uint8_t iv, uint8_t ev, uint8_t statUp, uint8_t level) {
    return ((2 * baseStat + iv + (ev / 4)) * level) / 100.0;
}

// Calculate an effective stat (besides HP) from its individual stat components
uint16_t getFinalStatNonHp(uint16_t baseStat, uint8_t iv, uint8_t ev, uint8_t statUp, uint8_t level) {
    // We reuse the HP formula, and just add 5
    return getFinalStatHp(baseStat, iv, ev, statUp, level) + 5;
}

// Total exp required to go from level 0 to targetLevel
uint32_t getTotalExpToLevel(uint8_t targetLevel, exp_group_t expGroup) {
    uint32_t targetLevelCubed = pow(targetLevel, 3);
    
    switch(expGroup) {
        case EXP_GROUP_FAST: {
            return 4 * targetLevelCubed / 5.0;
        }
        case EXP_GROUP_MEDIUM_FAST: {
            return targetLevelCubed;
        }
        case EXP_GROUP_FLUCTUATING: {
            if(targetLevel < 15) {
                return targetLevelCubed * (targetLevel + 1 / 3.0 + 24) / 50; // TODO: transcription error with the location of "24" with respect to the parenthesis in GameDesignDoc.md?
            } else if(targetLevel < 36) {
                return targetLevelCubed * (targetLevel + 14) / 50.0;
            } else {
                return targetLevelCubed * (targetLevel / 2.0 + 32) / 50;
            }
        }
        case EXP_GROUP_ERRATIC: {
            if(targetLevel < 50) {
                return targetLevelCubed * (100 - targetLevel) / 50.0;
            } else if(targetLevel < 68) {
                return targetLevelCubed * (150 - targetLevel) / 100.0;
            } else if(targetLevel < 98) {
                return targetLevelCubed * (1911 - 10 * targetLevel / 3.0) / 500;
            } else {
                return targetLevelCubed * (160 - targetLevel) / 100.0;
            }
        }
        case EXP_GROUP_MEDIUM_SLOW: {
            return 6 * targetLevelCubed / 5.0 - 15 * pow(targetLevel, 2) + 100 * targetLevel - 140;
        }
        case EXP_GROUP_SLOW: {
            return 5 * targetLevelCubed / 4.0;
        }
        default: {
            return getTotalExpToLevel(targetLevel, EXP_GROUP_SLOW);
        }
    }
}

// Exp required to go from currentLevel to currentLevel + 1
uint32_t getExpToNextLevel(uint8_t currentLevel, exp_group_t expGroup) {
    return getTotalExpToLevel(currentLevel + 1, expGroup) - getTotalExpToLevel(currentLevel, expGroup);
}

// Apply given amount of exp to a monster, and level it up if needed.
// Any excess exp is lost
void applyExpToMonster(monster_instance_t* monster, monster_instance_party_data_t* partyState, monster_final_stats_t* finalStats, uint32_t expToAdd) {
    exp_group_t expGroup = speciesDefs[monster->monsterId].expGroup;
    monster->exp = MIN(monster->exp + expToAdd, getTotalExpToLevel(MAX_LEVEL, expGroup));
    
    // TODO: exp earned dialogs
    
    while(monster->exp >= getExpToNextLevel(monster->level, expGroup) && monster->level < MAX_LEVEL) {
        monster->level++;
        // TODO: level up dialogs
        
        // TODO: update current HP and final stats
    }
}

// Apply given amounts of exp to individual party members, and level them up if needed
// Any excess exp is lost
void applyExpToParty(monster_instance_t party[], monster_instance_party_data_t partyState[], monster_final_stats_t finalStats[], uint32_t expToAdd[]) {
    for(int i = 0; i < PARTY_SIZE; i++) {
        applyExpToMonster(&party[i], &partyState[i], &finalStats[i], expToAdd[i]);
    }
}

// Allocate and apply given amount of exp to the monsters in the party, using the given distribution strategy.
// Any excess exp is allocated using fallback strategies, but lost if all monsters are max level
void applyExpToPartyByStrategy(monster_instance_t party[], monster_instance_party_data_t partyState[], monster_final_stats_t finalStats[], const uniq_arr_t* monstersParticipated, uint32_t exp, exp_strategy_t strategy) {
    uniq_arr_t monstersToGetExp;
    uniqArrInit(&monstersToGetExp, PARTY_SIZE, true);
    
    for(uint8_t i = 0; i < PARTY_SIZE; i++) {
        // Stop checking monsters once we hit a blank slot
        if(party[i].monsterId == 0) {
            break;
        }
        
        // If the monster is below the max level, add it to a list
        if (party[i].level < MAX_LEVEL) {
            uniqArrPut(&monstersToGetExp, i);
        }
    }
    
    // If there are no eligible monsters in the party, the exp is lost
    if(uniqArrEmpty(&monstersToGetExp)) {
        uniqArrFreeBuffer(&monstersToGetExp);
        return;
    }
    
    uint32_t expRemaining = exp;
    uint32_t expToAdd[PARTY_SIZE] = {0};
    uint32_t expToMaxLevel[PARTY_SIZE] = {0};
    exp_group_t expGroup[PARTY_SIZE];
    
    // Cache frequently needed variables
    for(int i = uniqArrLength(&monstersToGetExp); i >= 0; i--) {
        uint8_t curMonster;
        uniqArrGet(&monstersToGetExp, &curMonster, i);
        
        expGroup[curMonster] = speciesDefs[party[curMonster].monsterId].expGroup;
        
        // Check how much exp we can add without going over max level
        expToMaxLevel[curMonster] = getTotalExpToLevel(MAX_LEVEL, expGroup[curMonster]) - party[curMonster].exp;
    }
    
    uniq_arr_t monstersParticipatedToGetExp = {0};
    uniqArrCopy(&monstersParticipatedToGetExp, monstersParticipated, true);
    uniqArrIntersection(&monstersParticipatedToGetExp, &monstersToGetExp);
    
    switch(strategy) {
        case EXP_STRATEGY_LAST: // All exp to the monster that was in the battle when the enemy fainted
        {
            for(int i = uniqArrLength(&monstersParticipatedToGetExp) - 1; i >= 0 && expRemaining > 0; i--) {
                uint8_t curMonster;
                uniqArrGet(&monstersParticipatedToGetExp, &curMonster, i);
                
                // Aim to add all remaining exp, up to the maximum monster level
                uint32_t expToAddTemp = MIN(expRemaining, expToMaxLevel[curMonster]);
                
                expToAdd[curMonster] += expToAddTemp;
                expToMaxLevel[curMonster] -= expToAddTemp;
                expRemaining -= expToAddTemp;
                
                // Any remaining exp will be allocated in the next iteration(s) of the loop
            }
            
            if(expRemaining == 0) {
                break;
            }
            
            // If we still have exp left to allocate at this point, we need to allocate it to non-participating monsters
            uniqArrDifference(&monstersToGetExp, monstersParticipated);
            // TODO: allocate exp to non-participating monsters
            
            break;
        }
        case EXP_STRATEGY_PARTICIPATED: // Split exp evenly beteween all monsters that participated in the battle
        {
            ///// TODO: Code below here could be reused for other cases
            
            unsigned int numMonstersToGetExp = uniqArrLength(&monstersParticipatedToGetExp);
            
            if(numMonstersToGetExp > 0) {
            
                // Prepare a copy of this array that we can modify while the loop is iterating on the main copy
                uniq_arr_t monstersParticipatedToGetExpModified = {0};
                uniqArrCopy(&monstersParticipatedToGetExpModified, &monstersParticipatedToGetExp, true);
                
                // Whether or not exp was allocated this iteration of the loop. If no exp was allocated, there are no eligible participating monsters left
                bool allocatedExp;
                do {
                    allocatedExp = false;
                    uint32_t expPerMonster = expRemaining / numMonstersToGetExp; // We deal with the remainder later
                    
                    for(int i = numMonstersToGetExp; i >= 0; i--) {
                        uint8_t curMonster;
                        uniqArrGet(&monstersParticipatedToGetExp, &curMonster, i);
                        
                        // If there's exp evenly divided per monster, use that.
                        // Otherwise, whittle down the remainder 1 exp at a time.
                        // Only add exp up to the monster species' max.
                        uint32_t expToAddTemp = MIN(MAX(1, expPerMonster), expToMaxLevel[curMonster]);
                        
                        expToAdd[curMonster] += expToAddTemp;
                        expToMaxLevel[curMonster] -= expToAddTemp;
                        expRemaining -= expToAddTemp;
                        allocatedExp = allocatedExp && expToAddTemp > 0;
                        
                        // If this monster has reached its max exp...
                        if(expToMaxLevel[curMonster] == 0) {
                            // Remove it from the copy of the eligibility list for participated monsters
                            uniqArrRemove(&monstersParticipatedToGetExpModified, curMonster);
                            // Also remove it from the overall eligibility list
                            uniqArrRemove(&monstersToGetExp, curMonster);
                        }
                        
                        // If all exp has been allocated, skip iterating over the rest of the participating monsters
                        if(expRemaining == 0) {
                            break;
                        }
                    }
                    
                    // Apply the modifications made to the eligibility list
                    uniqArrCopy(&monstersParticipatedToGetExp, &monstersParticipatedToGetExpModified, true);
                    numMonstersToGetExp = uniqArrLength(&monstersParticipatedToGetExp);
                } while(expRemaining > 0 && numMonstersToGetExp > 0 && allocatedExp);
            }
            
            // If we still have exp left to allocate at this point, we need to allocate it to non-participating monsters
            uniqArrDifference(&monstersToGetExp, monstersParticipated);
            // TODO: allocate exp to non-participating monsters
            
            break;
        }
        case EXP_STRATEGY_CATCH_UP_SLOW: // Split exp between all monsters in the party, but give more to lower-level monsters. If all monsters are the same level, give more to monsters with less % progress to the next level. Then, try to keep all monsters at the same %
        {
            break;
        }
        case EXP_STRATEGY_CATCH_UP_FAST: // All exp to the lowest-level monster. If all monsters are the same level, give all to monsters with less % progress to the next level. Then, try to keep all monsters at the same %
        {
            break;
        }
        case EXP_STRATEGY_ALL: // Split exp evenly between all monsters in the party
        {
            break;
        }
    }
    
    applyExpToParty(party, partyState, finalStats, expToAdd);
    
    uniqArrFreeBuffer(&monstersToGetExp);
    uniqArrFreeBuffer(&monstersParticipatedToGetExp);
}

monster_instance_t* findEmptyMonsterSlot(monster_instance_t* party, monster_box_header_t* monsterBoxesWithHeaders[], uint8_t startBox) {
    monster_instance_t emptyMonster = {0};
    
    // First, try to find an empty slot in the party
    for(int i = 0; i < PARTY_SIZE; i++) {
        
        if(memcmp(&party[i], &emptyMonster, sizeof(monster_instance_t)) == 0) {
            return &party[i];
        }
    }
    
    // If the party is full, try to find a slot in the boxes, starting in the box the user viewed most recently, and wrapping around
    for(int i = 0; i < NUM_MONSTER_BOXES; i++) {
        uint8_t boxIdx = (startBox + i) % NUM_MONSTER_BOXES;
        monster_box_header_t* monsterBoxWithHeader = monsterBoxesWithHeaders[boxIdx];
        
        for(int monsterIdx = 0; monsterIdx < monsterBoxWithHeader->numMonsters; monsterIdx++) {
            monster_instance_t* monster = monsterBoxGet(monsterBoxWithHeader, monsterIdx);
            
            if(monster == NULL) {
                // TODO: what should the error behavior be here?
                continue;
            }
            
            if(memcmp(monster, &emptyMonster, sizeof(monster_instance_t)) == 0) {
                return monster;
            }
        }
    }
    
    // No empty slot was found
    return NULL;
}

bool catchMonster(const monster_instance_t* monster, monster_instance_t* party, monster_box_header_t* monsterBoxesWithHeaders[], dex_species_t* dex, uint8_t ballUsed) {
    monster_instance_t* dest = findEmptyMonsterSlot(party, monsterBoxesWithHeaders, 0 /*TODO: load last box index used from save data*/);
    
    if(dest == NULL) {
        return false;
    }
    
    memcpy(dest, monster, sizeof(monster_instance_t));
    
    // TODO: input nickname
    
    dest->trainerNameIdx = 0;
    // TODO: save ball used
    
    if(!dex[dest->monsterId].caught) {
        dex[dest->monsterId].caught = true;
        
        // TODO: show dex screen for this monster
    }
    
    return true;
}

bool tryCatchMonster(const monster_instance_t* monster, const monster_instance_party_data_t* monsterPartyState, const monster_final_stats_t* monsterFinalStats, uint8_t ballUsed) {
    // TODO: get the ball bonus for real
    double ballBonus = 1.0;
    
    double statusModifier;
    switch(monsterPartyState->statusCondition) {
        case STATUS_SLEEP:
        case STATUS_FREEZE:
        {
            statusModifier = 2.0;
            break;
        }
        case STATUS_BURN:
        case STATUS_PARALYZE:
        case STATUS_POISON:
        {
            statusModifier = 1.5;
            break;
        }
        case STATUS_NORMAL:
        default:
        {
            statusModifier = 1.0;
            break;
        }
    }
    
    uint32_t modifiedCatchRate = (3 * monsterFinalStats->hp - 2 * monsterPartyState->curHp) / (double) (3 * monsterFinalStats->hp) * speciesDefs[monster->monsterId].catchRate * ballBonus * statusModifier;
    
    if(modifiedCatchRate > 255) {
        return true;
    }
    
    uint32_t shakeChance = (uint32_t) round(1048560/sqrt(sqrt(16711680.0/modifiedCatchRate)));
    
    for(int i = 0; i < 4; i++) {
        if(esp_random() % 65536 >= shakeChance) {
            return false;
        }
        
        // TODO: render shake
    }
    
    return true;
}

bool tryRunAway(monster_instance_t* playerMonster, monster_final_stats_t* playerMonsterFinalStats, monster_instance_t* wildMonster, monster_final_stats_t* wildMonsterFinalStats, uint8_t attemptNum) {
    // TODO: abilities that guarantee success
    
    return esp_random() % 255 < playerMonsterFinalStats->speed * 128 / wildMonsterFinalStats->speed + 30 * attemptNum;
}

// Release a monster
bool releaseMonster(monster_instance_t* monster, names_header_t* monsterNamesWithHeader, names_header_t* trainerNamesWithHeader) {
    // Delete monster name, if it exists
    if(monster->nicknameIdx != 0 && !monsterNamesRemoveIdx(monsterNamesWithHeader, monster->nicknameIdx)) {
        return false;
    }
    
    // TODO: decrease reference to trainer name
    
    // Delete monster
    memset(monster, 0, sizeof(monster_instance_t));
    
    // The music fades, the curtains fall,
    // your quest comes to its end.
    // We laughed, we cried, but through it all,
    // I'm glad I called you "friend."
    
    return true;
    
    // The music swells across the hall, your quests have reached their end. something something and through it all, I'm glad I called you "friend."
    // The badges turn and crowds move on, Past booths of neon light. But though our battle-time is gone, Go find your spark tonight.
    // Amidst the masks and painted steel, Where fans and legends meet. I hope you find a bond that’s real, Along the city street.
    // The music swells across the hall, The speakers start to hum. I’ve loved our time, through it all, But now your time has come.
    // The speakers thrum beneath our feet, The crowd begins to roar. I leave you where the rhythms meet, To seek out something more.
    // Amidst the masks and neon glare, We’ve reached our journey's end. I release you to the vibrant air, My pixelated friend.
}

// TODO: find a better home for this function
uint32_t moneyLostOnBlackout(const monster_box_header_t* party, uint32_t curMoney, uint8_t numBadgesOwned) {
    uint8_t maxLevel = 0;
    
    for(uint8_t i = 0; i < PARTY_SIZE; i++) {
        maxLevel = MAX(maxLevel, monsterBoxGet(party, i)->level);
    }
    
    // Cap the money lost to a fixed percentage
    return MIN(maxLevel * numBadgesToBasePayout[numBadgesOwned], round(curMoney * MAX_MONEY_LOST_RATIO));
}

