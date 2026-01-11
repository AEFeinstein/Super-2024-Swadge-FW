#include <math.h>
#include "esp_random.h"
#include "unique_array.h"

#include "sm_monster.h"

void generateWildMonsterBySpecies(monster_instance_t* monster, monster_t* species, uint8_t levelMin, uint8_t levelMax) {
    monster->monsterId = species->monsterId;
    monster->nicknameIdx = MONSTER_NAME_IDX_NONE;
    monster->trainerNameIdx = TRAINER_NAME_IDX_NONE;
    monster->friendship = 0;
    monster->isFemale = (esp_random() % GENDER_RATIO_GENDERLESS) < species->genderRatio;
    monster->isShiny = (esp_random() % SHINY_DIVISOR) == 0;
    monster->exp = 0;
    monster->level = esp_random() % (levelMax - levelMin) + 1; // TODO: use a normal distribution?
    
    monster->ivs.hp = esp_random() % (MAX_IV + 1);
    monster->ivs.atk = esp_random() % (MAX_IV + 1);
    monster->ivs.def = esp_random() % (MAX_IV + 1);
    monster->ivs.spAtk = esp_random() % (MAX_IV + 1);
    monster->ivs.spDef = esp_random() % (MAX_IV + 1);
    monster->ivs.speed = esp_random() % (MAX_IV + 1);
    
    monster->evs.hp = 0;
    monster->evs.atk = 0;
    monster->evs.def = 0;
    monster->evs.spAtk = 0;
    monster->evs.spDef = 0;
    monster->evs.speed = 0;
    
    for(int i = 0; i < NUM_MOVES_PER_MONSTER; i++) {
        // TODO: generate moveset from species->levelUpMoves
        monster->moveIds[i] = 0;
        monster->ppUps[i] = 0;
    }
    
    monster->statUps.hp = 0;
    monster->statUps.atk = 0;
    monster->statUps.def = 0;
    monster->statUps.spAtk = 0;
    monster->statUps.spDef = 0;
    monster->statUps.speed = 0;
    
    // TODO: generate monster_instance_party_data_t
}

// Total exp required to go from level 0 to targetLevel
uint32_t getTotalExpToLevel(uint8_t targetLevel, exp_group_t expGroup) {
    uint32_t targetLevelCubed = pow(targetLevel, 3);
    
    switch(expGroup) {
        case EXP_GROUP_FAST:
            return 4 * targetLevelCubed / 5.0;
        case EXP_GROUP_MEDIUM_FAST:
            return targetLevelCubed;
        case EXP_GROUP_FLUCTUATING:
            if(targetLevel < 15) {
                return targetLevelCubed * (targetLevel + 1 / 3.0 + 24) / 50; // TODO: transcription error with the location of "24" with respect to the parenthesis in GameDesignDoc.md?
            } else if(targetLevel < 36) {
                return targetLevelCubed * (targetLevel + 14) / 50.0;
            } else {
                return targetLevelCubed * (targetLevel / 2.0 + 32) / 50;
            }
        case EXP_GROUP_ERRATIC:
            if(targetLevel < 50) {
                return targetLevelCubed * (100 - targetLevel) / 50.0;
            } else if(targetLevel < 68) {
                return targetLevelCubed * (150 - targetLevel) / 100.0;
            } else if(targetLevel < 98) {
                return targetLevelCubed * (1911 - 10 * targetLevel / 3.0) / 500;
            } else {
                return targetLevelCubed * (160 - targetLevel) / 100.0;
            }
        case EXP_GROUP_MEDIUM_SLOW:
            return 6 * targetLevelCubed / 5.0 - 15 * pow(targetLevel, 2) + 100 * n - 140
        case EXP_GROUP_SLOW:
            return 5 * targetLevelCubed / 4.0;
        //default:
        //    return getTotalExpToLevel(targetLevel, EXP_GROUP_SLOW);
    }
}

// Exp required to go from currentLevel to currentLevel + 1
uint32_t getExpToNextLevel(uint8_t currentLevel, exp_group_t expGroup) {
    return getTotalExpToLevel(currentLevel + 1, expGroup) - getTotalExpToLevel(currentLevel, expGroup);
}

void applyExpToPartyByStrategy(monster_instance_t* (party[]), monster_instance_party_data_t* (partyState[]), uniq_arr_t* monstersParticipated, uint32_t exp, exp_strategy_t strategy) {
    uniq_arr_t monstersToGetExp;
    uniqArrInit(monstersToGetExp, PARTY_SIZE, true);
    
    for(uint8_t i = 0; i < PARTY_SIZE; i++) {
        // Stop checking monsters once we hit a blank slot
        if((*party)[i].monsterId == 0) {
            break;
        }
        
        // If the monster is below the max level, add it to a list
        if ((*party)[i].level < MAX_LEVEL) {
            uniqArrPut(monstersToGetExp, i);
        }
    }
    
    // If there are no eligible monsters in the party, the exp is lost
    if(uniqArrEmpty(monstersToGetExp)) {
        uniqArrFree(monstersToGetExp);
        return;
    }
    
    uint32_t expRemaining = exp;
    
    switch(strategy) {
        case EXP_STRATEGY_LAST: // All exp to the monster that was in the battle when the enemy fainted
            for(unsigned int i = uniqArrLength(monstersParticipated) - 1; i >= 0 && expRemaining > 0; i--) {
                uint8_t curMonster;
                uniqArrGet(monstersParticipated, &curMonster, i);
                if(uniqArrSearch(&monstersToGetExp, NULL, curMonster)) {
                    exp_group_t expGroup = speciesDefs[party[curMonster].monsterId].expGroup;
                    
                    // Check how much exp we can add without going over max level
                    uint32_t expToMaxLevel = getTotalExpToLevel(MAX_LEVEL, expGroup) - party[curMonster].exp;
                    
                    // Aim to add all remaining exp, up to the maximum monster level
                    uint32_t expToAdd = MIN(expRemaining, expToMaxLevel);
                    
                    // Add the exp to the monster
                    party[curMonster].exp += expToAdd;
                    expRemaining -= expToAdd;
                    
                    while(party[curMonster].exp >= getExpToNextLevel(party[curMonster].level, expGroup) && party[curMonster].level < MAX_LEVEL) {
                        party[curMonster].level++;
                        // TODO: level up dialogs
                    }
                    
                    // Any remaining exp will be allocated in the next iteration(s) of the loop
                }
            }
            
            if(expRemaining == 0) {
                break;
            }
            
            // If we still have exp left to allocate at this point, we need to allocate it to non-participating monsters
            uniqArrDifference(&monstersToGetExp, monstersParticipated);
            TODO
        case EXP_STRATEGY_PARTICIPATED: // Split exp evenly beteween all monsters that participated in the battle
            uniq_arr_t monstersParticipatedToGetExp;
            uniqArrCopy(&monstersParticipatedToGetExp, monstersParticipated);
            uniqArrIntersection(&monstersParticipatedToGetExp, &monstersToGetExp);
            
            ///// TODO: Code below here could be reused for other cases
            
            unsigned int numMonstersToGetExp = uniqArrLength(&monstersParticipatedToGetExp);
            
            uint32_t expToAdd[PARTY_SIZE] = {0};
            uint32_t expToMaxLevel[PARTY_SIZE] = {0};
            exp_group_t expGroup[PARTY_SIZE];
            
            // Cache frequently needed variables
            for(unsigned int i = numMonstersToGetExp; i >= 0; i--) {
                uint8_t curMonster;
                uniqArrGet(&monstersParticipatedToGetExp, &curMonster, i);
                
                expGroup[curMonster] = speciesDefs[party[curMonster].monsterId].expGroup;
                
                // Check how much exp we can add without going over max level
                expToMaxLevel[curMonster] = getTotalExpToLevel(MAX_LEVEL, expGroup[curMonster]) - party[curMonster].exp;
            }
            
            bool allocatedExp;
            do {
                allocatedExp = false;
                uint32_t expPerMonster = expRemaining / numMonstersToGetExp; // We deal with the remainder later
                
                for(unsigned int i = numMonstersToGetExp; i >= 0; i--) {
                    uint8_t curMonster;
                    uniqArrGet(&monstersParticipatedToGetExp, &curMonster, i);
                    
                    uint32_t expToAddTemp = MIN(expPerMonster, expToMaxLevel[curMonster]);
                    expToAdd[curMonster] += expToAddTemp;
                    expToMaxLevel[curMonster] -= expToAddTemp;
                    expRemaining -= expToAddTemp;
                    allocatedExp = allocatedExp && expToAddTemp > 0;
                }
            } while(expRemaining >= numMonstersToGetExp && allocatedExp);
            
            // Split up the remainder exp that can't be evenly divided between monsters, giving priority to the last participated
            for(unsigned int i = numMonstersToGetExp; i >=0; i--) {
                uint8_t extraExp = 0;
                if(expPerMonsterRemainder > 0) {
                    extraExp = 1;
                }
                
                // TODO
                if() {
                    expPerMonsterRemainder--;
                }
            }
            
            // TODO: allocate exp to non-participating monsters
            
            // TODO: apply exp from expToAdd[] using the indices from monstersToGetExp
            
            // TODO: level up
            
            break;
        EXP_STRATEGY_CATCH_UP_SLOW, // Split exp between all monsters in the party, but give more to lower-level monsters. If all monsters are the same level, give more to monsters with less % progress to the next level. Then, try to keep all monsters at the same %
        EXP_STRATEGY_CATCH_UP_FAST, // All exp to the lowest-level monster. If all monsters are the same level, give all to monsters with less % progress to the next level. Then, try to keep all monsters at the same %
        EXP_STRATEGY_ALL, // Split exp evenly between all monsters in the party
    }
    
    uniqArrFree(monstersToGetExp);
}

void releaseMonster(monster_instance_t* monster, names_header_t* monsterNames, names_header_t* trainerNames) {
    // TODO: delete monster name
    // TODO: decrease reference to trainer name
    memset(monster, 0, sizeof(monster_instance_t));
}
