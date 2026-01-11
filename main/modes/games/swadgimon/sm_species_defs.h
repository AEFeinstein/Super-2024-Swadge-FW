#ifndef _SM_SPECIES_DEFS_H
#define _SM_SPECIES_DEFS_H

typedef enum {
    ID_NONE,
    ID_GENERIMON,
    NUM_SPECIES_INCL_NULL
} species_id_t;

static const monster_t speciesDefs[] = {
    {
        .monsterId = ID_GENERIMON;
        .name = "Generimon",
        .description = "Bland and nonspecific.",
        .types[2] = {TYPE_NORMAL, TYPE_NONE},
        .catchRate = 255,
        .expYield = 1,
        .evYields = ,
        .genderRatio = GENDER_RATIO_50M,
        .baseStats = ,
        .expGroup = EXP_GROUP_FAST,
        .evolutions[3] = ,
        .learnableTMs = ,
        .learnableHMs = ,
        .otherValidMoves = ,
        .levelUpMoves = ,
    },
}

#endif

