#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "ggCommonTypes.h"
#include "trophy.h"
#include "macros.h"

//==============================================================================
// Defines
//==============================================================================

// Scores
#define OHP_SCORE 1000
#define NNP_SCORE  990
#define NFP_SCORE  950

// Milestone percents
#define MILE_ACC 33
#define MILE_ROUNDS 25
#define MILE_WORST 99

//==============================================================================
// Consts
//==============================================================================

const trophyData_t ggTrophies[] = {
    {
        .title       = "Maybe talk to a doctor",
        .description = "Complete 20 rounds of Gotta Go!",
        .image       = GG_20GAMES_WSG,
        .type        = TROPHY_TYPE_ADDITIVE,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 20,
    },
    {
        .title       = "Drank a Bit Too Much",
        .description = "Beat 10 levels in a row",
        .image       = GG_10ROUNDS_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 10,
    },
    {
        .title       = "Hydrohomie",
        .description = "Beat 20 levels in a row",
        .image       = GG_20ROUNDS_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 20,
    },
    {
        .title       = "Hyperhydrosis",
        .description = "Beat 30 levels in a row",
        .image       = GG_30ROUNDS_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 30,
    },
    {
        .title       = "Laser beam",
        .description = "Beat 15+ levels with a perfect accuracy",
        .image       = GG_ONE_HUNDRED_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 15,
    },
    {
        .title       = "Sniper",
        .description = "Beat 15+ levels with 99+%% accuracy",
        .image       = GG_NINETY_NINE_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 15,
    },
    {
        .title       = "Sharpshooter",
        .description = "Beat 15+ levels with 95+%% accuracy",
        .image       = GG_NINETY_FIVE_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 15,
    },
    {
        .title       = "Emergency Plan",
        .description = "When all options are filled or broken, use a stall",
        .image       = GG_STALL_CORRECT_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_HARD,
        .maxVal      = 1,
    },
    {
        .title       = "Stall Hogger",
        .description = "Attempt to use a stall with no uses left",
        .image       = GG_STALL_INCORRECT_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_MEDIUM,
        .maxVal      = 1,
    },
    {
        .title       = "Socialite",
        .description = "Pick the worst available option 5 times in a row",
        .image       = GG_WORST_OPTION_WSG,
        .type        = TROPHY_TYPE_PROGRESS,
        .difficulty  = TROPHY_DIFF_EXTREME,
        .maxVal      = 5,
    },
    {
        .title       = "Aim at the dot",
        .description = "Activate helper mode for the first time",
        .image       = GG_HELPER_MODE_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
    },
    {
        .title       = "DOWN WITH THE ESRB",
        .description = "Find the manifesto",
        .image       = GG_MANI_PEDI_WSG,
        .type        = TROPHY_TYPE_TRIGGER,
        .difficulty  = TROPHY_DIFF_EASY,
        .maxVal      = 1,
        .hidden      = true,
    },
};

const trophySettings_t ggTrophySettings = {
    .drawFromBottom   = false,
    .staticDurationUs = DRAW_STATIC_US * 2,
    .slideDurationUs  = DRAW_SLIDE_US,
    .namespaceKey     = ggModeName,
};

const trophyDataList_t ggTrophyDate = {
    .length   = ARRAY_SIZE(ggTrophies),
    .list     = ggTrophies,
    .settings = &ggTrophySettings,
};

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    T_ROUNDS_20,
    T_LEVELS_10,
    T_LEVELS_20,
    T_LEVELS_30,
    T_OHP,
    T_NNP,
    T_NFP,
    T_USE_STALL_GOOD,
    T_USE_STALL_BAD,
    T_WORST_OPTIONS,
    T_HELP_MODE,
    T_MANIFESTO,
} ggTrophyNames_t;