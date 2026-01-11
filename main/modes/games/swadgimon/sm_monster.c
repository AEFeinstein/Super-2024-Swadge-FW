#include "sm_monster.h"

static void generateWildMonsterBySpecies(monster_instance_t* monster, monster_t species) 
    monster->monsterId = species->id;
    monster->nicknameIdx = MONSTER_NAME_IDX_NONE;
    monster->trainerNameIdx = PLAYER_NAME_IDX_NONE;
    monster->friendship = 0;
    monster->isFemale = (esp_random() % GENDER_RATIO_GENDERLESS) < species->genderRatio;
    monster->isShiny = (esp_random() % SHINY_DIVISOR) == 0;
    
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