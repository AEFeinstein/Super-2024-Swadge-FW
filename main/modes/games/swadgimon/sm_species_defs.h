#ifndef _SM_SPECIES_DEFS_H
#define _SM_SPECIES_DEFS_H

typedef enum {
    ID_NONE,
    ID_GENERIMON,
    NUM_SPECIES_INCL_NULL
} species_id_t;

static const monster_t speciesDefs[] = {
    {
        .monsterId = ID_GENERIMON,
        .name = "Generimon",
        .description = "Bland and nonspecific.",
        .types = {TYPE_NORMAL, TYPE_NONE},
        .catchRate = 255,
        .expYield = 1,
        .evYields = {1, 0, 0, 0, 0, 0},
        .genderRatio = GENDER_RATIO_50M,
        .baseStats = {1, 1, 1, 1, 1, 1},
        .expGroup = EXP_GROUP_FAST,
        .evolutions = {0},
        .learnableTMs = {0},
        .learnableHMs = {0},
        .otherValidMoves = {0},
        .levelUpMoves = {{1, 1}},
    },
}

#endif

