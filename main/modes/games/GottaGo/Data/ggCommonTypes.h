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
// Enums
//==============================================================================

/// @brief Game state
typedef enum
{
    GG_WARNING,
    GG_SPLASH,
    GG_MENU,
    GG_READY,
    GG_GAME,
    GG_CHOICE,
    GG_WON,
    GG_LOST,
    GG_RULES,
    GG_HIGHSCORE,
    GG_OPTIONS,
} ggState_t;

/// @brief Trophy names
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

/// @brief Types of pants
typedef enum
{
    GG_PANTS,
    GG_SHORTS,
    GG_SKIRT,
    GG_DOWN,
    GG_UNDERWEAR,
    GG_NO_PANTS,
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

/// @brief Warning screen text
typedef enum
{
    GG_WARNING_TEXT,
    GG_WARNING_MESSAGE,
    GG_WARNING_MANI,
} ggWarningText_t;

/// @brief Level text
typedef enum
{
    GG_TEXT_READY,
    GG_TEXT_3,
    GG_TEXT_2,
    GG_TEXT_1,
    GG_TEXT_PAUSED,
    GG_TEXT_INSTR_PAUSE,
    GG_TEXT_GOOD_1,
    GG_TEXT_GOOD_2,
    GG_TEXT_INSTR_GOOD,
    GG_TEXT_BAD_1,
    GG_TEXT_BAD_2,
    GG_TEXT_BAD_3,
    GG_TEXT_INSTR_BAD,
} ggLevelText_t;

/// @brief Menu text
typedef enum
{
    GG_TEXT_MENU_PLAY,
    GG_TEXT_MENU_RULES,
    GG_TEXT_MENU_HS,
    GG_TEXT_MENU_OPTIONS,
    GG_TEXT_MENU_COUNT
} ggMenuText_t;

/// @brief Options text
typedef enum
{
    GG_TEXT_OPTIONS_HELPER = GG_TEXT_MENU_COUNT,
    GG_TEXT_OPTIONS_TOUCH,
    GG_TEXT_OPTIONS_COUNT
} ggOptionsText_t;

/// @brief Extras
typedef enum
{
    GG_TEXT_QUIT = GG_TEXT_OPTIONS_COUNT,
    GG_TEXT_BACK,
    GG_TEXT_ON,
    GG_TEXT_OFF,
} ggSettingsText;

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
    // WSGs
    wsg_t* backgroundImages; ///< Images used in the background of the game
    wsg_t* urinalImages;     ///< Images for generating urinals
    wsg_t* npcImages;        ///< Images for drawing NPCs
    wsg_t* titleImages;      ///< Images used in the splash screen
    wsg_t* uiImages;         ///< Images used by the UI

    // Fonts
    font_t titleFont;        ///< Font for large text
    font_t titleFontOutline; ///< Outline for large text
    font_t descFont;         ///< Font for smaller text

    // SFX

    // Main
    ggState_t state;   ///< Game state
    int64_t timer;     ///< Timer, used for all timing tasks
    int64_t timeLimit; ///< When timer is triggered
    int selection;     ///< Current selection, menu, urinal, etc.

    // Settings
    bool touch;  ///< If using the touch input
    bool helper; ///< If using the helper mode

    // Level State
    int numActive;                   ///< Number of active urinals
    int stallUses;                   ///< How many stall uses are remaining
    int stallReq;                    ///< Current requirement to get a new stall use
    ggUrinal_t urinals[MAX_URINALS]; ///< All urinal data
    bool pause;                      ///< If the game is paused

    // Current Score
    int64_t timeRemaining; ///< Time remaining once option is selected
    int numLevels;         ///< Current number of level inside a round
    int accScore;          ///< How accurate the player is
    int totalScore;        ///< Total score, unmodified
    int adjScore;          ///< Total score, including modifier
    bool hasLost;          ///< If level resulted in a failure
    bool stallUsed;        ///< If a stall was used this level

    // Trophies
    int accLevel1; ///< Highest accuracy requirement
    int accLevel2; ///< Middle accuracy requirement
    int accLevel3; ///< Lowest accuracy requirement
    int numWorst;  ///< Number of levels picked worst option

    // Drawing
    bool toggle;             ///< Used for all toggle
    wsgPalette_t npcPalette; ///< Color converter
} ggData_t;