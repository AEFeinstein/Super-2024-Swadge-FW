#ifndef _CGROVE_TYPES_H_
#define _CGROVE_TYPES_H_

// Defines
//==============================================================================
// Swadge
#define V_SCREEN_SIZE   240
#define H_SCREEN_SIZE   280

// Chowa Garden
#define MAX_CHOWA       5
#define MAX_PREV_GUESTS 3
#define USERNAME_CHARS  32

// Strings
//==============================================================================
static const char cGroveTitle[] = "Chowa Grove";            // Game title

// Menus
static const char cGroveStrPlay[] = "Play with Chowa!";     // Go to grove
static const char cGroveStrCompete[] = "Compete!";          // Go to competition menu
static const char cGroveStrShops[] = "Go to the shops";     // Go to Shop menu
static const char cGroveStrOnline[] = "Go Online";          // Access online function Menu

static const char cGroveCompStrFight[] = "Chowa Combat";    // Go to fight
static const char cGroveCompStrRace[] = "Quadrathalon";     // Go to Race
static const char cGroveCompStrPerf[] = "Karaoke";          // Go to Performance

static const char cGroveShopStrGroc[] = "Grocery";          // Place to buy food
static const char cGroveShopStrToy[] = "Toy store";         // Place to buy Toys
static const char cGroveShopStrBook[] = "Bookstore";        // Place to buy skill books
static const char cGroveShopStrSchool[] = "School";         // Place to drop of Chowa to increase skills

static const char cGroveOLStrViewProf[] = "View profiles";                  // View player profile and most recent connected profiles
static const char cGroveOLStrEditProf[] = "Edit Public Profile";            // Allows user to edit their profile
static const char cGroveOLStrEnable[] = "Enable/Disable online features";   // Enables or disables online 

static const char cGrovePronoun[] = "Pronouns";
static const char cGroveMood[] = "Mood";
static const char cGroveMoodColon[] = "Mood: ";
static const char cGroveUser[] = "Username";
static const char cGroveShowAwards[] = "Show stats";

static const char cGrovePronounHe[] = "He/Him";
static const char cGrovePronounShe[] = "She/Her";
static const char cGrovePronounThey[] = "They/them";
static const char cGrovePronounOther[] = "Other";

static const char cGroveMoodStrNeutral[] = "Neutral";
static const char cGroveMoodStrHappy[] = "Happy";
static const char cGroveMoodStrSad[] = "Sad";
static const char cGroveMoodStrAngry[] = "Angry";
static const char cGroveMoodStrConfused[] = "Confused";
static const char cGroveMoodStrSick[] = "Sick";
static const char cGroveMoodStrSurprised[] = "Surprised";

/* 
// Grove
static const char cGrovePlayStrInteract[] = "Play";         // Pet, pick up, throw, see stats
static const char cGrovePlayStrGive[] = "Give an item";     // selects an item to drop into Grove

// NPC data FIXME: Convert to struct
static const char cGroveNPC1Name[] = "Pixel";
static const char cGroveNPC2Name[] = "Poe";
static const char cGroveNPC3Name[] = "Pango";
static const char cGroveNPC4Name[] = "Dr. Garbotnik";
// TODO: Add ~6 NPCs
// TODO: Add NPC descriptions
// TODO: Add NPC win strings
// TODO: Add NPC lose strings
// TODO: Add NPC competitiion strings

// Debug
static const char DebugText[] = "TEXT_UNDEFINED";
 */

// Enums
//==============================================================================
typedef enum
{
    MENU,
    PROFILE,
    SUBPROFILE,
    SHOP,
    SCHOOL,
    GROVE,
    FIGHT,
    PERFORM,
    RACE,
} State_t;

typedef enum
{
    HAPPY,
    ANGRY,
    SAD,
    CONFUSED,
    SURPRISED,
    SICK,
    NEUTRAL,
} Mood_t;

typedef enum
{
    SHY,
    BRASH,
    BORING,
    DUMB,
    CRYBABY,
    SMART,
    OVERLYCAUTIOUS,
    CARELESS,
    KIND,
    AGGRESSIVE,
} Personality_t;

typedef enum 
{
    DEMON_HORNS,
    TOP_HAT,
} Cosmetics_t;

typedef enum
{
    CRAYONS,
    BALL,
    TOYSWORD,
    REALKNIFE,
    CAKE,
    SOUFFLE,
    COOKIE,
    STATBOOK_STR,
    STATBOOK_AGI,
    STATBOOK_SPD,
    STATBOOK_CHA,
    STATBOOK_STA,
} Gifts_t;

typedef enum
{
    HE_HIM,
    SHE_HER,
    THEY_THEM,
    OTHER
} Pronouns_t;

// Structs
//============================================================================
typedef struct
{
    uint16_t color;         // Color for Chowa
    int8_t age;             // 0-255;   Age in hours
    int8_t maxAge;          // 0-255;   Maximum age in hours
    int8_t strength;        // 0-255;   Strength stat
    int8_t agility;         // 0-255;   Agility stat
    int8_t speed;           // 0-255;   Speed stat
    int8_t charisma;        // 0-255;   Charisma stat
    int8_t stamina;         // 0-255;   Stamina stat
    int8_t activeCosmetic;  // Currently active cosmetic
    int8_t stomach;         // How full the stomach is
    Personality_t pers;     // Personality
    Mood_t currMood;        // Active mood of the Chowa
    Cosmetics_t wornHat;    // Current worn cosmetic
    Gifts_t heldGift;       // GIft to hand to another Chowa
    //Possible addition: Families/Parents/Children
} Chowa_t;

typedef struct
{
    char* name;             // Name string
    char* description;      // Description string
    int8_t profileIndex;    // Index to the profile sprite
    char* startMsg;         // Message used at the start of competitions
    char* winMsg;           // Message used when theNPC wins
    char* loseMsg;          // Message used when the player wins
    int8_t numChowa;        // Number of NPC chowa
    Chowa_t NPC_Chowa[3];   // NPC's Chowa
} NPC_t;

typedef struct
{
    char username[USERNAME_CHARS];  // Player's username
    Pronouns_t pronouns;            // Player's chosen pronouns
    Chowa_t chowa[MAX_CHOWA];       // Player's Chowa
    Mood_t mood;                    // Player's set mood
} playerProfile_t;

typedef struct
{
    // Swadge data
    buttonEvt_t evt;        // State of the buttons
    bool online;            // True is online, False is offline

    // State tracking
    State_t currState;          // Current game state
    int8_t customMenuSelect;    // Current selection on custom menu

    // Swadge structs
    menu_t* cGroveMenu;             // Main menu for cGrove
    menuLogbookRenderer_t* rndr;    // Logbook renderer

    // Assets
    font_t menuFont;        // Menu font FIXME: Change to new sonic font
    wsg_t mood_icons[7];    // Icons of various moods
    wsg_t arrow;

    // Chowa Grove data
    playerProfile_t player;                     // Player struct
    playerProfile_t guests[MAX_PREV_GUESTS];    // List of guest players
    int8_t numOfOnlineGuests;                   // Number of guests currently available
    Cosmetics_t cosmeticInv[255];               // List of owned cosmetics
} cGrove_t;

#endif