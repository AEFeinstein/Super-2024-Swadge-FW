/**
 * @file cg_Typedef.h
 * @author Jeremy Stintzcum (jeremy.stintzcum@gmail.com)
 * @brief Types for Chowa Grove
 * @version 0.1
 * @date 2024-09-19
 *
 * @copyright Copyright (c) 2024
 *
 */
#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "swadge2024.h"
#include "wsgPalette.h"

//==============================================================================
// Defines
//==============================================================================
#define CG_MAX_STR_LEN 17

//==============================================================================
// Items
//==============================================================================

// Structs ==============================
typedef struct
{
    char name[CG_MAX_STR_LEN]; ///< Name
    rectangle_t aabb;          ///< Position and bounding box
    wsg_t spr;                 ///< Spr for item
    bool active;               ///< If item slot is being used
} cgItem_t;

typedef struct
{
    char name[CG_MAX_STR_LEN]; ///< Name
    wsg_t* spr;                ///< Sprite
} cgWearable_t;

//==============================================================================
// Chowa
//==============================================================================

// Defines =============================
#define CG_MAX_CHOWA  10 // Max number of Chowa allowed on a swadge
#define CG_STAT_COUNT 6  // Number of stats

// Enums ===============================
typedef enum
{
    CG_NEUTRAL,
    CG_HAPPY,
    CG_WORRIED,
    CG_SAD,
    CG_ANGRY,
    CG_CONFUSED,
    CG_SURPRISED,
    CG_SICK,
} cgMoodEnum_t;

typedef enum
{
    CG_AGGRESSIVE,
    CG_BORING,
    CG_BRASH,
    CG_CARELESS,
    CG_CRY_BABY,
    CG_DUMB,
    CG_KIND,
    CG_OVERLY_CAUTIOUS,
    CG_SHY,
    CG_SMART,
} cgChowaPersonality_t;

typedef enum
{
    CG_HEALTH,
    CG_STRENGTH,
    CG_AGILITY,
    CG_SPEED,
    CG_CHARISMA,
    CG_STAMINA,
} cgChowaStat_t;

typedef enum
{
    CG_NORMAL,
    CG_BIRD,
    CG_DONUT,
    CG_LUMBERJACK_1,
    CG_LUMBERJACK_2,
    CG_ZAPPY,
} cgColorType_t;

typedef enum
{
    CHOWA_STATIC,
    CHOWA_WANDER,
    CHOWA_CHASE,
    CHOWA_HELD,
} cgChowaStateGarden_t;

// Structs ==============================
typedef struct
{
    // System
    bool active; ///< If Chowa slot is being used. Only set to true if data has been set.

    // Base data
    char name[17];
    int8_t age;                         ///< Current age of the Chowa
    int8_t maxAge;                      ///< Maximum Chowa age. 4 hours of in game time
    uint8_t playerAffinity;              ///< How much Chowa likes the player
    cgMoodEnum_t mood;                  ///< Current mood of the Chowa
    cgChowaPersonality_t pers;          ///< Chowa's personality
    cgChowaStat_t stats[CG_STAT_COUNT]; ///< Array containing stat information

    // Color data
    // Note: Palette must be initialized for all Chowa, regardless or the colors will be screwy
    cgColorType_t type; ///< Type of Chowa
    wsgPalette_t color; ///< If Normal type, color scheme

    // Garden
    rectangle_t aabb;            ///< Position and bounding box for grabbing
    bool holdingItem;            ///< If Chowa is holding an item
    cgItem_t* heldItem;          ///< Pointer to the held item
    cgChowaStateGarden_t gState; ///< Behavior state in the garden
    vec_t targetPos;             ///< Target position when moving
    int32_t waitTimer;           ///< generic timer for waiting between states
} cgChowa_t;

//==============================================================================
// NPCs
//==============================================================================

typedef struct
{
    char name[CG_MAX_STR_LEN]; ///< Name of the NPC
    wsg_t* icon;               ///< Icons for dialogue
    // Text for competitions
    cgChowa_t chowa[3]; ///< NPCs CHowa
} cgNPC_t;

//==============================================================================
// Submode generics
//==============================================================================

// Enums ===============================
typedef enum
{
    CG_P1,
    CG_P2,
} cgPlayers_t;

typedef enum
{
    CG_P1_WIN,
    CG_P2_WIN,
    CG_DRAW,
} cgWinLoss_t;

typedef enum
{
    CG_BEGINNER,
    CG_VERY_EASY,
    CG_EASY,
    CG_MEDIUM,
    CG_HARD,
    CG_VERY_HARD,
    CG_EXPERT,
} cgAIDifficulty_t;

// Structs ==============================
typedef struct
{
    char matchTitle[CG_MAX_STR_LEN];     ///< Title of the match
    char playerNames[2][CG_MAX_STR_LEN]; ///< Player names
    char chowaNames[6][CG_MAX_STR_LEN];  ///< Up to 6 Chowa participate
    cgColorType_t colorType[6];          ///< Type of Chowa
    wsgPalette_t palettes[6];            ///< Colors of the Chowa for drawing
    cgWinLoss_t result[3];               ///< Results of all three matches
    int16_t timer[3];                    ///< Time per round in seconds
} cgRecord_t;

//==============================================================================
// Garden
//==============================================================================

// Defines =============================
#define CG_FIELD_OBJ_COUNT  64
#define CG_FIELD_BOUNDARY   32
#define CG_FIELD_ITEM_LIMIT 10
#define CG_FIELD_HEIGHT     750
#define CG_FIELD_WIDTH      750

// Structs ==============================
typedef struct
{
    rectangle_t aabb; ///< Camera position and bounding box for draw calls
} cgGardenCamera_t;

typedef struct
{
    wsg_t spr;        ///< Sprite for object
    rectangle_t aabb; ///< Position and collision box
} cgGardenObject_t;

typedef struct
{
    cgGardenObject_t staticObjects[CG_FIELD_OBJ_COUNT]; ///< Static objects littering the field
    cgGardenCamera_t cam;                               ///< Camera
} cgField_t;

typedef struct
{
    // Objects
    cgField_t field;                     ///< Field object
    cgItem_t items[CG_FIELD_ITEM_LIMIT]; ///< List of items in the garden

    // Cursor
    rectangle_t cursorAABB; ///< Cursor position and bounding box
    bool holdingItem;       ///< If the player is holding an item
    cgItem_t* heldItem;     ///< The held item
    bool holdingChowa;      ///< If the pl;ayer is holding a Chowa
    cgChowa_t* heldChowa;   ///< The held Chowa
} cgGarden_t;

//==============================================================================
// Sparring
//==============================================================================

// Defines =============================
#define CG_SPAR_MAX_RECORDS 10 ///< Max number of saved matches
#define CG_MAX_READY_VALUE  64 ///< How large the Ready bars are

// Enums ===============================

typedef enum
{
    CG_SPAR_SPLASH,
    CG_SPAR_MENU,
    CG_SPAR_SCHEDULE,
    CG_MATCH_PREP,
    CG_SPAR_MATCH,
    CG_SPAR_MATCH_RESULTS,
    CG_SPAR_TUTORIAL,
    CG_SPAR_BATTLE_RECORD,
} cgSparState_t;

typedef enum
{
    CG_SPAR_UNREADY,   ///< Building up for an attack
    CG_SPAR_READY,     ///< Ready to attack
    CG_SPAR_EXHAUSTED, ///< Gain Stamina
    CG_SPAR_HIT,       ///< State used to animate being hit
    CG_SPAR_ATTACK,    ///< State used to animate Attacking
    CG_SPAR_DODGE_ST,  ///< State used to animate dodging
    CG_SPAR_NOTHING,   ///< State for when teh CHowa do nothing
    CG_SPAR_WIN,       ///< Once round ends, Animate in victory pose
    CG_SPAR_LOSE,      ///< Once round ends, Animate in lose pose (Crying?)
} cgMatchChowaState_t;

typedef enum
{
    CG_SPAR_PUNCH,
    CG_SPAR_KICK,
    CG_SPAR_FAST_PUNCH,
    CG_SPAR_JUMP_KICK,
    CG_SPAR_HEADBUTT,
    CG_SPAR_DODGE,
    CG_SPAR_UNSET, ///< Used to avoid any specific moves being prioritized in readiness system
} cgRPSState_t;

// Structs ==============================
typedef struct
{
    wsg_t* spr;     ///< Image
    vec_t startPos; ///< Starting x and y positions
    vec_t pos;      ///< Position on screen
    vec_t speed;    ///< Speed
} cgSparBGObject_t;

typedef struct
{
    cgChowa_t* chowa;              ///< Chowa object
    cgMatchChowaState_t currState; ///< Current Chowa state
    cgRPSState_t currMove;         ///< Current selected move
    int16_t maxStamina;            ///< Max stamina of each Chowa
    int16_t stamina;               ///< Stamina of both Chowa for stamina bars
    int16_t readiness;             ///< How ready each Chowa is
    int16_t HP;                    ///< Current HP for this Chowa
    int16_t maxHP;                 ///< Max HP for this Chowa
    int32_t updateTimer;           ///< Used for readiness updates
    bool doneAnimating;            ///< Currently animating
} cgSparChowaData_t;

typedef struct
{
    cgAIDifficulty_t aiDifficulty; ///< Difficulty
    bool pickedMove;               ///< If a move has already been picked
    bool init;                     ///< Initialized
    int8_t movesPicked;            ///< Num of move already picked
    cgRPSState_t prevState;        ///< Last move the AI chose
    cgRPSState_t prevMoves[20];    ///< All the previous moves the player has taken
    int64_t timer;                 ///< Timer for button presses
} cgSparAI_t;

typedef struct
{
    // Data
    char matchName[CG_MAX_STR_LEN]; ///< Name of the current match
    int8_t round;                   ///< The round of the fight

    // State
    cgSparChowaData_t chowaData[2]; ///< Extended Chowa data
    bool paused;                    ///< If the match is paused
    bool online;                    ///< If match is online
    bool resolve;                   ///< Marks that the match should be resolved

    // AI
    cgSparAI_t ai;

    // Match ended
    bool done;               ///< If match if finished
    cgWinLoss_t finalResult; ///< The ultimate result of the match

    // Match time
    int64_t usTimer; ///< Microsecond timer
    int16_t timer;   ///< Round timer
    int16_t maxTime; ///< Max time allowed for the round

    // Animations
    bool animDone; ///< If Animation is done
    bool wasCrit;  ///< If Chowa was hit while unready
} cgMatch_t;

typedef struct
{
    // Assets
    // Audio
    // - BGM, menus
    // - BGM, match
    // - Combat sounds
    //   - Pain sounds
    //   - Impact sounds
    //   - Gong crash
    //   - Countdown noises
    //   - Cheer noises

    // BG Sprites
    wsg_t dojoBG;       ///< Dojo Background image
    wsg_t* dojoBGItems; ///< Dojo BG items
    // UI Sprites
    wsg_t arrow; ///< Arrow sprite
    // - Punch icon
    // - Kick icon
    // NPC sprites

    // Fonts
    font_t sparTitleFont;        ///< Font used for larger text
    font_t sparTitleFontOutline; ///< Outline for title font
    font_t sparRegFont;          ///< Regular text

    // Spar
    cgSparState_t state; ///< Active state
    int64_t timer;       ///< Timer for animations
    bool toggle;         ///< Toggles on timer

    // Menu
    menu_t* sparMenu;              ///< Menu object
    menuManiaRenderer_t* renderer; ///< Renderer

    // Match
    cgMatch_t match; ///< Match object

    // Battle Record
    cgRecord_t sparRecord[CG_SPAR_MAX_RECORDS]; ///< List of battle records
    int8_t recordSelect;                        ///< Which record is currently active
    int8_t roundSelect;                         ///< Which round of the record is currently selected

    // Input

    // LEDs

} cgSpar_t;

//==============================================================================
// Mode Data
//==============================================================================

// Defines =============================
// FIXME: Get rid of these
#define CG_CHOWA_EXPRESSION_COUNT 3
#define CG_GARDEN_ITEMS_COUNT     1
#define CG_GARDEN_STATIC_OBJECTS  1
#define CG_GARDEN_CURSORS         1

// Enums ===============================
typedef enum
{
    CG_MAIN_MENU,
    CG_GROVE,
    CG_SPAR,
    CG_RACE,
    CG_PERFORMANCE,
} cgMainState_t;

// Structs =============================
typedef struct
{
    // Assets
    // ========================================================================
    // Fonts
    font_t menuFont; ///< Main font

    // Modes
    cgGarden_t garden; ///< Garden data
    cgSpar_t spar;     ///< Spar data

    // State
    cgMainState_t state; ///< Main mode state
    bool loaded; ///< if the state has already been loaded

    // Settings
    bool touch;  ///< Touch controls active
    bool online; ///< If online features are enabled

    // Chowa
    cgChowa_t chowa[CG_MAX_CHOWA]; ///< List of Chowa
} cGrove_t;