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
    int8_t numOfUses;          ///< Number of uses before being used up
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
#define CG_MAX_CHOWA  5  // Max number of Chowa allowed on a swadge
#define CG_STAT_COUNT 6  // Number of stats
#define CG_ADULT_AGE  64 // Age before a chowa becomes an adult

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
    // Emotes
    CG_ANIM_ANGRY,
    CG_ANIM_CONFUSED,
    CG_ANIM_CRY,
    CG_ANIM_DISGUST,
    CG_ANIM_FEAR,
    CG_ANIM_FLAIL,
    CG_ANIM_GIVE_UP,
    CG_ANIM_HAPPY,
    CG_ANIM_SAD,
    CG_ANIM_SHOCKED,

    // Moving
    CG_ANIM_WALK_UP,
    CG_ANIM_WALK_DOWN,
    CG_ANIM_WALK_RIGHT,
    CG_ANIM_WALK_LEFT,
    CG_ANIM_SWIM_RIGHT,
    CG_ANIM_SWIM_LEFT,
    CG_ANIM_CLIMB,
    CG_ANIM_JUMP,

    // Falling
    CG_ANIM_TRIP_UP,
    CG_ANIM_TRIP_RIGHT,
    CG_ANIM_TRIP_LEFT,

    // Attacking
    CG_ANIM_HEADBUTT_RIGHT,
    CG_ANIM_HEADBUTT_LEFT,
    CG_ANIM_KICK_RIGHT,
    CG_ANIM_KICK_LEFT,
    CG_ANIM_PUNCH_RIGHT,
    CG_ANIM_PUNCH_LEFT,

    // Damage
    CG_ANIM_HIT_RIGHT,
    CG_ANIM_HIT_LEFT,

    // Using Items
    CG_ANIM_DRAW,
    CG_ANIM_EAT,
    CG_ANIM_GIFT,
    CG_ANIM_READ,
    CG_ANIM_SWORD,
    CG_ANIM_THROW_RIGHT,
    CG_ANIM_THROW_LEFT,

    // Other
    CG_ANIM_DANCE,
    CG_ANIM_PET,
    CG_ANIM_SING,
    CG_ANIM_SIT,
} cgChowaAnimIdx_t;

typedef enum
{
    CG_ADULT,
    CG_CHILD,
} cgChowaAnimAge_t;

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
    CG_KING_DONUT,
    CG_NUM_TYPES, ///< Move if more get added
    CG_NORMAL,
    CG_CHO,
    CG_RED_LUMBERJACK,
    CG_GREEN_LUMBERJACK,
    CG_KOSMO,
    CG_LILB,
} cgColorType_t;

// Structs ==============================
typedef struct
{
    // System
    bool active; ///< If Chowa slot is being used. Only set to true if data has been set.

    // Base data
    char name[CG_MAX_STR_LEN];
    char owner[CG_MAX_STR_LEN];         ///< Name of the owning player
    int8_t age;                         ///< Current age of the Chowa
    uint8_t playerAffinity;             ///< How much Chowa likes the player
    cgMoodEnum_t mood;                  ///< Current mood of the Chowa
    cgChowaStat_t stats[CG_STAT_COUNT]; ///< Array containing stat information

    // Color data
    // Note: Palette must be initialized for all Chowa, regardless or the colors will be screwy
    cgColorType_t type; ///< Type of Chowa
    wsgPalette_t color; ///< If Normal type, color scheme
} cgChowa_t;

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
    CG_NUM_DIFF, // For loops
} cgAIDifficulty_t;

// Structs ==============================
typedef struct
{
    bool active;                     ///< If record contains real data
    char matchTitle[CG_MAX_STR_LEN]; ///< Title of the match
    cgChowa_t* chowa[6];             ///< Chowa Data
    cgWinLoss_t result[3];           ///< Results of all three matches
    int16_t timer[3];                ///< Time per round in seconds
    int8_t numRounds;                ///< How many rounds are in the record
} cgRecord_t;

//==============================================================================
// Garden
//==============================================================================

// Defines =============================
#define CG_GROVE_MAX_ITEMS       10 ///< Max number of items in the grove
#define CG_GROVE_MAX_GUEST_CHOWA 5  ///< Maximum number of Chowa allowed to be in the grove at once

#define CG_GROVE_SCREEN_BOUNDARY 32 ///< How close the cursor can get to the edge of the screen

// Enum =================================
typedef enum
{
    CG_GROVE_FIELD,
    CG_GROVE_MENU,
    CG_GROVE_SHOP,
    CG_GROVE_INVENTORY,
    CG_GROVE_VIEW_STATS,
    CG_GROVE_ABANDON,
    CG_GROVE_TUTORIAL,
    CG_KEYBOARD_WRITE_NAME,
} cgGroveState_t;

typedef enum
{
    CHOWA_IDLE,      ///< Doing nothing. Get new behavior
    CHOWA_STATIC,    ///< Standing in place
    CHOWA_WALK,      ///< Walking, running, swimming, struggling to swim towards a target
    CHOWA_CHASE,     ///< Follow Other chowa, object, or cursor
    CHOWA_GRAB_ITEM, ///< Grab an item if close enough
    CHOWA_DROP_ITEM, ///< Drop an item occasionally
    CHOWA_USE_ITEM,  ///< Use an item held in Chowa's possession
    CHOWA_BOX,       ///< Does sparring type moves
    CHOWA_SING,      ///< Sings
    CHOWA_DANCE,     ///< Dancing
    CHOWA_TALK,      ///< Talks with another Chowa
    CHOWA_HELD,      ///< Held, cannot move
    CHOWA_GIFT,      ///< Receiving a gift/head pats
    CHOWA_PET,       ///< Chowa is being pet
} cgChowaStateGarden_t;

typedef enum
{
    CG_BOOK_AGI,
    CG_BOOK_CHA,
    CG_BOOK_SPD,
    CG_BOOK_STA,
    CG_BOOK_STR,
    CG_BALL,
    CG_CRAYONS,
    CG_TOY_SWORD,
    CG_KNIFE,
    CG_CAKE,
    CG_SOUFFLE,
    CG_CHOWA_EGG,
    CG_MAX_TYPE_ITEMS
} cgGroveItemEnum_t;

// Structs ==============================
typedef struct
{
    int16_t money;                         ///< Money
    uint8_t quantities[CG_MAX_TYPE_ITEMS]; ///< Item qtys
} cgInventory_t;

typedef struct
{
    rectangle_t aabb;     ///< Position and bounding box
    int64_t despawnTimer; ///< Time left until money despawns
    int8_t animFrame;     ///< Sparkle animation frames
    bool active;          ///< If the ring is active
} cgGroveMoney_t;

typedef struct
{
    bool active;        ///< if slot is in use
    cgColorType_t type; ///< Used for color data
    int64_t timer;      ///< us since last stage
    int16_t stage;      ///< Current stage
    rectangle_t aabb;   ///< Position of the Egg
} cgEgg_t;

typedef struct
{
    // Basic data
    cgChowa_t* chowa; ///< Main Chowa data
    cgEgg_t* egg;     ///< If not null, Egg is in this slot
    rectangle_t aabb; ///< Position and bounding box for grabbing

    // Items
    cgItem_t* heldItem; ///< Pointer to the held item

    // AI
    cgChowaStateGarden_t gState;    ///< Behavior state in the garden
    cgChowaStateGarden_t nextState; ///< Cued state after arriving at target
    int64_t timeLeft;               ///< Set to number of seconds a behavior can take
    int64_t nextTimeLeft;           ///< Time left on next state
    vec_t targetPos;                ///< Position to head to
    float precision;                ///< How precise the position needs to be
    int64_t moodTimer;              ///< How long before mood shifts
    int64_t ageTimer;               ///< Timer to update age of the Chowa

    // Animations
    int16_t angle;         ///< Angle that the Chowa is moving at
    int8_t animFrame;      ///< Frame that the animation is on
    int64_t frameTimer;    ///< Timer until the next frame triggers
    int8_t animIdx;        ///< which animation is being played
    bool flip;             ///< If image needs to be flipped manually
    bool hasPartner;       ///< If Chowa has a partner for talking or boxing
    bool ballInAir;        ///< If ball is being thrown.
    bool ballFlip;         ///< If the ball is going left
    int64_t ballTimer;     ///< Timer for the ball animations
    int16_t ballAnimFrame; ///< Why yes, Balls should be their own object.
    int16_t ySpd;          ///< Y speed of the ball.
} cgGroveChowa_t;

typedef struct
{
    // Assets
    // Grove WSGs
    wsg_t groveBG;         ///< Grove background
    wsg_t* cursors;        ///< Cursor sprites
    wsg_t* angerParticles; ///< Anger particle sprites
    wsg_t* questionMarks;  ///< Question mark sprites
    wsg_t* notes;          ///< Musical notes
    wsg_t* speechBubbles;  ///< Speech Bubbles for Chowa
    wsg_t* itemsWSGs;      ///< Item sprites
    wsg_t* eggs;           ///< Un-cracked eggs
    wsg_t* crackedEggs;    ///< Cracked eggs
    // Audio
    midiFile_t bgm; ///< Main BGM for the Grove

    // State
    cgGroveState_t state; ///< Current state of the grove
    int64_t saveTimer;    ///< How long until CHowa are automatically saved
    bool tutorial;        ///< If tutorial has been run

    // Field data
    cgItem_t items[CG_GROVE_MAX_ITEMS];                            ///< Items present in the Grove
    rectangle_t waterBoundary;                                     ///< Boundary boxes
    cgGroveChowa_t chowa[CG_MAX_CHOWA + CG_GROVE_MAX_GUEST_CHOWA]; ///< List of all chowa in the garden
    bool bgmPlaying;                                               ///< If the BGM is active
    cgInventory_t inv;                                             ///< Inventory struct
    cgGroveMoney_t ring;                                           ///< Ring available to collect
    cgEgg_t unhatchedEggs[CG_MAX_CHOWA];                           ///< Array of un-hatched eggs
    int8_t hatchIdx;                                               ///< Used for text input

    // Menu
    menu_t* menu;                  ///< Shop menu object
    menuManiaRenderer_t* renderer; ///< Menu renderer
    // Items
    int8_t shopSelection;      ///< Index of the shop currently set / CUrrently selected field item
    int8_t itemSelection;      ///< Selection of what to put on field
    int8_t groveActiveItemIdx; ///< Index of the next item slot to load item into
    // Chowa
    bool confirm; ///< Used to gate accidentally deleting chowa or kicking guests

    // Player resources
    rectangle_t camera;        ///< In-garden camera viewport
    rectangle_t cursor;        ///< Cursor position and bounding box
    bool holdingItem;          ///< If the player is holding an item
    cgItem_t* heldItem;        ///< The held item
    bool holdingChowa;         ///< If the player is holding a Chowa
    cgGroveChowa_t* heldChowa; ///< The held Chowa
    bool isPetting;            ///< If the petting gesture should be going on
    int64_t pettingTimer;      ///< How long before petting resets

    // Tutorial
    int8_t tutorialPage; ///< Drawn page
} cgGrove_t;

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
    CG_MATCH_PREP,
    CG_SPAR_MATCH,
    CG_SPAR_TUTORIAL,
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

    // Status bars
    int16_t maxStamina;  ///< Max stamina of each Chowa
    int16_t stamina;     ///< Stamina of both Chowa for stamina bars
    int16_t readiness;   ///< How ready each Chowa is
    int16_t HP;          ///< Current HP for this Chowa
    int16_t maxHP;       ///< Max HP for this Chowa
    int32_t updateTimer; ///< Used for readiness updates

    // Animations
    bool doneAnimating; ///< Currently animating
    int64_t animTimer;  ///< Used to count miliseconds for animations
    int8_t animFrame;   ///< Current frame of the animation
    int8_t hOffset;     ///< Offset for animations
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
    cgRecord_t data;            ///< Match data
    cgSparChowaData_t chowa[2]; ///< Extended Chowa object
    int8_t round;               ///< The round of the fight

    // State
    bool paused;  ///< If the match is paused
    bool resolve; ///< Marks that the match should be resolved
    bool done;    ///< If match is finished

    // AI
    cgSparAI_t ai;

    // Match time
    int64_t usTimer; ///< Microsecond timer
    int16_t timer;   ///< Round timer
    int16_t maxTime; ///< Max time allowed for the round

    // Animations
    bool animDone;        ///< If Animation is done
    int64_t endGameTimer; ///< Accumulates until game ends
} cgMatch_t;

typedef struct
{
    // Assets
    // BG Sprites
    wsg_t dojoBG;       ///< Dojo Background image
    wsg_t* attackIcons; ///< Attack and dodge icons
    // Music
    midiFile_t sparBGM; ///< Music

    // Spar
    cgSparState_t state; ///< Active state
    int64_t timer;       ///< Timer for animations
    int8_t animFrame;    ///< Frame for animations
    bool toggle;         ///< Toggles on timer

    // Menu
    menu_t* sparMenu;              ///< Menu object
    menuManiaRenderer_t* renderer; ///< Renderer

    // Match setup
    int8_t numActiveChowa;                               ///< Number of active chowa
    int8_t activeChowaIdxs[CG_MAX_CHOWA];                ///< Indexes of active chowa
    char activeChowaNames[CG_MAX_CHOWA][CG_MAX_STR_LEN]; ///< Names of Chowa currently active
    int8_t optionSelect;                                 ///< Currently selected option
    int8_t chowaSelect;                                  ///< Currently selected Chowa
    int8_t aiSelect;                                     ///< Currently selected AI difficulty
    int8_t timerSelect;                                  ///< Currently selected timer length

    // Match
    cgMatch_t match;    ///< Match object
    cgChowa_t opponent; ///< Current opponent
} cgSpar_t;

//==============================================================================
// Mode Data
//==============================================================================

// Enums ===============================
typedef enum
{
    CG_MAIN_MENU,
    CG_GROVE,
    CG_SPAR,
    CG_FIRST_RUN,
    CG_ERASE,
} cgMainState_t;

// Structs =============================
typedef struct
{
    bool touch;      ///< Touch controls for Grove
    bool online;     ///< If online features are enabled
    bool itemText;   ///< If item text should be drawn
    bool chowaNames; ///< If Chowa's show have their names drawn in Grove
} cgSettings_t;

typedef struct
{
    // Assets
    // ========================================================================
    // Fonts
    font_t menuFont;         ///< Main font
    font_t largeMenuFont;    ///< Larger font, same style
    font_t titleFont;        ///< Font for titles
    font_t titleFontOutline; ///< OPutline of above

    // WSGs
    wsg_t* title;                      ///< Title screen sprites
    wsg_t* chowaWSGs[CG_NUM_TYPES][2]; ///< Chowa sprites
    wsg_t arrow;                       ///< Arrow str
    // NPC sprites

    // Audio
    midiPlayer_t* mPlayer; ///< Midi Player
    midiFile_t menuBGM;    ///< Menu BGM

    // Modes
    cgGrove_t grove; ///< Garden data
    cgSpar_t spar;   ///< Spar data

    // State
    cgMainState_t state; ///< Main mode state
    bool unload;         ///< if the state is ready to unload

    // title screen
    bool titleActive;  ///< If title screen is active
    int64_t timer;     ///< Timer for animations
    vec_t cloudPos;    ///< Position of the cloud
    int8_t animFrame;  ///< Current frame of the animation;
    int8_t titleFrame; ///< Frame of title animation

    // Menu
    menu_t* menu;                  ///< Main menu
    menuManiaRenderer_t* renderer; ///< Menu renderer

    // Settings
    cgSettings_t settings; ///< Settings struct

    // Chowa
    cgChowa_t chowa[CG_MAX_CHOWA];              ///< List of Chowa
    cgChowa_t guests[CG_GROVE_MAX_GUEST_CHOWA]; ///< Guest Chowa

    // Text Entry buffer
    char buffer[CG_MAX_STR_LEN];

    // Player data
    char player[CG_MAX_STR_LEN];
} cGrove_t;