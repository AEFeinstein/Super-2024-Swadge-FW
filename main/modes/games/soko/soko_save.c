#include "soko.h"


void sokoLoadLevelSolvedState(soko_abs_t* soko){
    //todo: automatically split for >32, >64 levels using 2 loops.

    int lvs = 0;
    readNvs32("sklv1",&lvs);
    //i<32...
    for (size_t i = 0; i < SOKO_LEVEL_COUNT; i++)
    {
        soko->levelSolved[i] = (1 & lvs>>i) == 1;
    }
    //now the next 32 bytes!
    // readNvs32("sklv2",&lvs);
    // for (size_t i = 32; i < SOKO_LEVEL_COUNT || i < 64; i++)
    // {
    //     soko->levelSolved[i] = (1 & lvs>>i) == 1;
    // }

    //etc. Probably won't bother cleaning it into nested loop until over 32*4 levels...
    //so .. never?
}

/// @brief Called on 'resume' from the menu.
/// @param soko 
void sokoLoadGameplay(soko_abs_t* soko){
    //current level
    //current level entity positions
    //etc.
    sokoLoadLevelSolvedState(soko);
}


void sokoSetLevelSolvedState(soko_abs_t* soko, uint16_t levelIndex, bool solved){
    printf("save level solved status %d\n", levelIndex);
    //todo: changes a single levels bool in the sokoSolved array,
    soko->levelSolved[levelIndex] = true;

    int section = levelIndex/32;
    int index = levelIndex;
    int lvs = 0;

    if (section == 0)
    {
        readNvs32("sklv1",&lvs);
    }else if(section == 1){
        readNvs32("sklv2",lvs);
        index -= 32;
    }//else, 64, 

    //write the bit.
    if(solved){
        //set bit
        lvs = lvs | (1 << index);
    }else{
        //clear bit
        lvs = lvs & ~(1 << index);
    }

    //write the bit out to data.
    if (section == 0)
    {
        writeNvs32("sklv1",lvs);
    }else if(section == 1){
        writeNvs32("sklv2",lvs);
    }
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