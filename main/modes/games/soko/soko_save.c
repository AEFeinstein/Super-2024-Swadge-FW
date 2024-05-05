#include "soko.h"

void sokoLoadLevelSolvedState(soko_abs_t* soko){
    //todo: sets the sokoSolved bool[] from the disc.
    for (size_t i = 0; i < SOKO_LEVEL_COUNT; i++)
    {
        soko->levelSolved[i] = false;
    }
    
}

void sokoSetLevelSolvedState(soko_abs_t* soko, uint16_t levelIndex, bool solved){
    printf("save level solved status %d\n", levelIndex);
    //todo: changes a single levels bool in the sokoSolved array,
    soko->levelSolved[levelIndex] = true;
    //and saves it to the disc.
}

void sokoSolveCurrentLevel(soko_abs_t* soko){
    if(soko->currentLevelIndex == 0){
        //overworld level.
        return;
    }else{
        sokoSetLevelSolvedState(soko,soko->currentLevelIndex,true);
    }
}

//Saving Progress
//soko->overworldX
//soko->overworldY
//current level? or just stick on overworld?

//current level progress (all entitity positions/data, entities array. non-entities comes from file.)
//euler encoding? (do like picross level?)