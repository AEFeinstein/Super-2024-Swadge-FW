#ifndef _SM_SPECIES_DEFS_H
#define _SM_SPECIES_DEFS_H

#include <stdbool.h>
#include <stddef.h>

#include "sm_monster.h"
#include "sm_move.h"

typedef enum {
    ID_NONE,
    ID_GENERIMON,
    ID_PERFECAT,
    NUM_SPECIES_INCL_NULL,
} species_id_t;

typedef enum {
    MOVE_NONE,
    MOVE_PUNCH,
    NUM_MOVES_INCL_NULL,
} move_id_t;

static const monster_evolution_t generimonEvolutions[] = {
    {
        .method = EVO_LEVEL,
        .parameter = 5,
        .targetMonsterId = ID_PERFECAT,
    }
};

static const move_t movePunch = {
    .id = MOVE_PUNCH,
    .name = "Punch",
    .description = "It's a punch. Why do you need a description? Are you recording me? Turn that thing off righ-",
    .effect = 0,
    .power = 20,
    .type = TYPE_NORMAL,
    .accuracy = 100,
    .pp = 35,
    .effectChance = 0,
    .priority = 0,
    .makesContact = true,
    .affectedByProtect = true,
    .affectedByMagicCoat = false,
    .affectedBySnatch = false,
    .usableByMirrorMove = true,
};

static const monster_level_up_move_t generimonLevelUpMoves[] = {
    {
        .moveId = MOVE_PUNCH,
        .level = 3,
    },
};

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
        .evolutions = generimonEvolutions,
        .learnableTMs = NULL,
        .learnableHMs = NULL,
        .otherValidMoves = NULL,
        .levelUpMoves = generimonLevelUpMoves,
    },
    {
        .monsterId = ID_PERFECAT,
        .name = "Perfecat",
        .description = "The ideal cat.",
        .types = {TYPE_NORMAL, TYPE_NONE},
        .catchRate = 35,
        .expYield = 3,
        .evYields = {1, 0, 1, 0, 0, 0},
        .genderRatio = GENDER_RATIO_50M,
        .baseStats = {2, 2, 2, 2, 2, 2},
        .expGroup = EXP_GROUP_MEDIUM_FAST,
        .evolutions = NULL,
        .learnableTMs = NULL,
        .learnableHMs = NULL,
        .otherValidMoves = NULL,
        .levelUpMoves = NULL,
    },
};

#endif

