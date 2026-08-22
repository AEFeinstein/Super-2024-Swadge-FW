#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "swadge.h"

//==============================================================================
// Defines
//==============================================================================

// General Settings
#define GG_SECOND 1000000

// Game settings
#define MAX_URINALS 7 ///< Based on width of the screen/available space

//==============================================================================
// Consts
//==============================================================================

const char ggModeName[] = "Gotta Go!";

// Color Arrays
static const paletteColor_t skinColors[] = {
    c555, c333, c444, c023, c402, c233, c343,
};
static const paletteColor_t shirtColors[] = {
    c500, c050, c005, c440, c204, c404, c044,
};
static const paletteColor_t shirtAccentColors[] = {
    c400, c040, c004, c330, c103, c303, c033,
};
static const paletteColor_t pantsColors[] = {
    c024, c330, c222, c224, c503, c240, c031,
};
static const paletteColor_t pantsAccentColors[] = {
    c012, c220, c111, c113, c402, c130, c020,
};
static const paletteColor_t shoeColors[] = {
    c210,
    c000,
    c111,
    c300,
};

//==============================================================================
// Enums
//==============================================================================

/// @brief Types of pants
typedef enum
{
    GG_PANTS,
    GG_SHORTS,
    GG_SKIRT,
    GG_DOWN,
    GG_UNDERWEAR,
    GG_NAKED,
    GG_PANTS_COUNT
} ggPants_t;

/// @brief Types of dividers
typedef enum
{
    GG_DIV_FULL,
    GG_DIV_TOP,
    GG_DIV_BOTTOM,
    GG_DIV_NONE,
    GG_DIV_COUNT
} ggDivider_t;

/// @brief Types of Puddles
typedef enum
{
    GG_PEE_NONE,
    GG_PEE_SMALL,
    GG_PEE_MED,
    GG_PEE_LARGE,
    GG_PEE_COUNT
} ggPeePuddle_t;

/// @brief Types of Cracks
typedef enum
{
    GG_NO_CRACK,
    GG_CRACK_1,
    GG_CRACK_2,
    GG_CRACK_3,
    GG_CRACK_COUNT
} ggCracks_t;

/// @brief Types of Major issues
typedef enum
{
    GG_BROKEN_BOWL,
    GG_BROKEN_DRAIN,
    GG_PLUGGED,
    GG_OOO,
    GG_MAJOR_COUNT
} ggMajor_t;

//==============================================================================
// Structs
//==============================================================================

/// @brief NPC data
typedef struct
{
    int8_t randOffset;
    // Functional
    uint8_t active : 1;
    // Colors
    uint8_t skinColor  : 3;
    uint8_t shirtColor : 3;
    uint8_t pantsColor : 3;
    uint8_t shoeColor  : 2;
    // Attributes
    uint8_t small : 1;
    uint8_t stink : 1;
    uint8_t shirt : 1;
    uint8_t pants : 3;
    uint8_t shoes : 1;
} ggNPC_t;

/// @brief Urinal data
typedef struct
{
    int height; // Height of actual unit (default: 35)
    // Bonuses
    uint8_t autoFlush : 1;
    // Minor
    uint8_t cracks    : 2;
    uint8_t graffiti  : 3;
    uint8_t waterLeak : 1;
    // Major
    uint8_t brokenBowl   : 1;
    uint8_t brokenDrain  : 1;
    uint8_t outOfOrder   : 1;
    uint8_t pluggedDrain : 1;
    // Additional factors
    uint8_t puddle  : 2;
    uint8_t divider : 2;
    uint8_t small   : 1;
    ggNPC_t npc;
} ggUrinal_t;

typedef struct
{
    // Assets

    // Main
    ggState_t state; ///< Game state
    int64_t timer;   ///< Timer, used for all timing tasks
    int selection;   ///< Current selection, menu, urinal, etc.

    // Game State
    int numActive;                   ///< Number of active urinals
    int stallUses;                   ///< How many stall uses are remaining
    ggUrinal_t urinals[MAX_URINALS]; ///< All urinal data
    int64_t timeLimit;               ///< How much time user has to select a urinal
    int64_t timeRemaining;           ///< Time remaining once option is selected

    // Current Score
    int numLevels;  ///< Current number of level inside a round
    int accScore;   ///< How accurate the player is
    int totalScore; ///< Total score, unmodified
    int adjScore;   ///< Total score, including modifier
    int stallReq;   ///< Current requirement to get a new stall use
    bool stallUsed; ///< If a stall was used this level
    bool hasLost;   ///< If level resulted in a failure

    // Trophies
    int accLevel1; ///< Highest accuracy requirement
    int accLevel2; ///< Middle accuracy requirement
    int accLevel3; ///< Lowest accuracy requirement
    int numWorst;  ///< Number of levels picked worst option

    // TODO: Reorder
    font_t normalFont_;
    font_t normalFontOutline_;
    font_t smallFont_;
    bool helper_;
    bool touch_;
    bool splashToggle_;
    wsg_t* splashImgs_;
    bool pause_;
    bool timeOut_;
    wsg_t* backgroundImages_;
    wsg_t* toiletImages_;
    wsg_t* npcImages_;
    wsgPalette_t npcPalette_;
    wsg_t* uiImages_;
} ggData_t;