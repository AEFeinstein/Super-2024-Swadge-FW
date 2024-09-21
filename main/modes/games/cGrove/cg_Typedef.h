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
// Defines
//==============================================================================

#include "swadge2024.h"

//==============================================================================
// Items
//==============================================================================

// Structs ==============================
typedef struct
{
    char name[16];
    rectangle_t aabb;
    wsg_t spr;
    bool active;
} cgItem_t;

typedef struct
{
    wsg_t* spr;
} cgWearable_t;

//==============================================================================
// Chowa
//==============================================================================

// Defines =============================
#define CG_MAX_CHOWA 10

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
    cgChowaStat_t statIdx; ///< Used to identify, get strings, etc
    uint16_t value;        ///< Current value of the stat (0-65535)
} cgSkill_t;

typedef struct
{
    // System
    bool active; ///< If Chowa slot is being used. Only set to true if data has been set.

    // Base data
    int8_t age;                ///< Current age of the Chowa
    int8_t maxAge;             ///< Maximum Chowa age. 4 hours of in game time
    int8_t PlayerAffinity;     ///< How much Chowa likes the player
    cgMoodEnum_t mood;         ///< Current mood of the Chowa
    cgChowaPersonality_t pers; ///< Chowa's personality
    cgChowaStat_t stats[5];    ///< Array containing stat information

    // Color data
    // Note: Palette must be initialized for all Chowa, regardless or the colors will be screwy
    cgColorType_t type; ///< Type of Chowa
    // wsgPalette_t color; ///< If Normal type, color scheme

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
    // Name
    char name[16];
    // Icon
    wsg_t* icon;
    // Text for competitions
    // Chowa
    cgChowa_t chowa[3];
} cgNPC_t;

//==============================================================================
// Garden
//==============================================================================

// Defines =============================
#define CG_FIELD_OBJ_COUNT  64
#define CG_FIELD_BOUNDARY   32
#define CG_FIELD_ITEM_LIMIT 10
#define CG_FIELD_HEIGHT     750
#define CG_FIELD_WIDTH      750

// TODO: Label and organize

typedef struct
{
    rectangle_t aabb;
} cgGardenCamera_t;

typedef struct
{
    wsg_t spr;
    rectangle_t aabb;
} cgGardenObject_t;

typedef struct
{
    cgGardenObject_t staticObjects[CG_FIELD_OBJ_COUNT];
    cgGardenCamera_t cam;
} cgField_t;

typedef struct
{
    // Objects
    cgField_t field;
    cgItem_t items[CG_FIELD_ITEM_LIMIT];

    // Cursor
    rectangle_t cursorAABB;
    bool holdingItem;
    cgItem_t* heldItem;
    bool holdingChowa;
    cgChowa_t* heldChowa;
} cgGarden_t;

//==============================================================================
// Sparring
//==============================================================================

// Defines =============================
#define CG_SPAR_BG_ITEMS 2

// Enums ===============================
typedef enum
{
    CG_SPAR_SPLASH,
    CG_SPAR_MENU,
    CG_SPAR_SCHEDULE,
    CG_SPAR_MATCH,
    CG_SPAR_TUTORIAL,
    CG_SPAR_BATTLE_RECORD,
} cgSparState_t;

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
    cgChowa_t* participants; ///< Chowa data
    uint8_t stamina[2];      ///< Stamina of both Chowa for stamina bars
    int8_t round;            ///< The round of the fight
    int16_t timer;           ///< Round timer
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

    // Sprites
    wsg_t* dojoBG;      ///< Dojo Background image
    wsg_t* dojoBGItems; ///< Dojo BG items
    // UI Sprites
    // - Punch icon
    // - Kick icon
    // NPC sprites

    // Fonts

    // Spar
    cgSparState_t state; ///< Active state

    // Menu
    menu_t* sparMenu;              ///< Menu object
    menuManiaRenderer_t* renderer; ///< Renderer

    // Match
    cgMatch_t match; ///< Match object

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
    font_t menuFont;
    // Sprites
    wsg_t gardenSpr[CG_GARDEN_STATIC_OBJECTS];
    wsg_t cursors[CG_GARDEN_CURSORS];
    wsg_t items[CG_GARDEN_ITEMS_COUNT];
    wsg_t chowaExpressions[CG_CHOWA_EXPRESSION_COUNT];

    // Modes
    cgGarden_t garden;
    cgSpar_t spar;

    // State
    cgMainState_t state;

    // Settings
    bool touch; // If using the touch pad for controls

    // Chowa
    cgChowa_t chowa[CG_MAX_CHOWA];
} cGrove_t;