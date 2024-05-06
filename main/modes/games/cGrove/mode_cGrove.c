/**
 * @file mode_cGrove.c
 * @author Jeremy Stintzcum (Jeremy.Stintzcum@gmail.com)
 * @brief A small Chao garden inspiried program
 * @version 0.1
 * @date 2024-05-04
 * 
 * @copyright Copyright (c) 2024
 * 
 */

// Includes
//==============================================================================
 #include "mode_cGrove.h"

// Defines
//==============================================================================
#define MAX_CHOWA 5
#define MAX_GUEST_CHOWA 3

// Function Prototypes
//==============================================================================
static void cGroveMainLoop(int64_t);
static void cGroveExitMode(void);
static void cGroveEnterMode(void);
static void cGroveMenuCB(const char*, bool, uint32_t);
static void cGroveBackgroundDrawCallback(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t);
static void cGroveToggleOnlineFeatures(void);
static void cGroveEspNowRecvCb(const esp_now_recv_info_t*, const uint8_t*, uint8_t, int8_t);
static void cGroveEspNowSendCb(const uint8_t*, esp_now_send_status_t);

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

static const char cGroveOLStrEditProf[] = "Edit Public Profile";            // Allows user to edit their profile
static const char cGroveOLStrEnable[] = "Enable/Disable online features";   // Enables or disables online 

static const char cGrovePronoun[] = "Pronouns";
static const char cGroveMood[] = "Mood";
static const char cGroveUser[] = "Username";

static const char cGrovePronounHe[] = "He/Him";
static const char cGrovePronounShe[] = "She/Her";
static const char cGrovePronounThey[] = "They/them";
static const char cGrovePronounOther[] = "Other";

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
    HAPPY,
    ANGRY,
    SAD,
    CONFUSED,
    FEARFUL,
    SURPRISED,
    DISGUSTED,
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
//==============================================================================
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
    char* username;             // Player's username
    Pronouns_t pronouns;        // Player's chosen pronouns
    Chowa_t Chowa[MAX_CHOWA];   // Player's Chowa
    Mood_t mood;                // Player's set mood
} playerProfile_t;

typedef struct
{
    // Swadge data
    uint16_t buttonState;   // State of the buttons
    bool online;            // True is online, False is offline

    // UI data
    uint8_t cursorPos;  // Position of the menu cursor

    // Menu
    menu_t* cGroveMenu;             // Main menu for cGrove
    menuLogbookRenderer_t* rndr;    // Logbook renderer
    font_t menuFont;                 // Menu font

    // Chowa Grove data
    playerProfile_t player;                     // Player struct
    Chowa_t guestChowaList[MAX_GUEST_CHOWA];    // List of guest Chowas
    Cosmetics_t cosmeticInv[255];               // List of owned cosmetics
} cGrove_t;

// Variables
//==============================================================================
 swadgeMode_t cGroveMode = {
    .modeName                 = cGroveTitle,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = cGroveEnterMode,
    .fnExitMode               = cGroveExitMode,
    .fnMainLoop               = cGroveMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = cGroveBackgroundDrawCallback,
    .fnEspNowRecvCb           = cGroveEspNowRecvCb,
    .fnEspNowSendCb           = cGroveEspNowSendCb,
    .fnAdvancedUSB            = NULL,
    .fnDacCb                  = NULL,
};

cGrove_t* grove = NULL;

// Main Functions
//==============================================================================

static void cGroveEnterMode(void)
{
    // Mode memory allocation
    grove = calloc(1, sizeof(cGrove_t));

    // Load a font
    loadFont("logbook.font", &grove->menuFont, false);

    // Menu initialization
    grove->cGroveMenu = initMenu(cGroveTitle, cGroveMenuCB);
    addSingleItemToMenu(grove->cGroveMenu, cGroveStrPlay);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveStrCompete);
    addSingleItemToMenu(grove->cGroveMenu, cGroveCompStrFight);
    addSingleItemToMenu(grove->cGroveMenu, cGroveCompStrRace);
    addSingleItemToMenu(grove->cGroveMenu, cGroveCompStrSing);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveStrShops);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrGroc);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrToy);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrBook);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrSchool);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveStrOnline);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveOLStrEditProf);
    addSingleItemToMenu(grove->cGroveMenu, cGroveUser);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGrovePronoun);
    addSingleItemToMenu(grove->cGroveMenu, cGrovePronounHe);
    addSingleItemToMenu(grove->cGroveMenu, cGrovePronounShe);
    addSingleItemToMenu(grove->cGroveMenu, cGrovePronounThey);
    addSingleItemToMenu(grove->cGroveMenu, cGrovePronounOther);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveMood);
    // TODO: Add mood selections
    //addSingleItemToMenu(grove->cGroveMenu, );
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    addSingleItemToMenu(grove->cGroveMenu, cGroveOLStrEnable);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->rndr = initMenuLogbookRenderer(&grove->menuFont);
}

static void cGroveExitMode(void)
{
    deinitMenu(grove->cGroveMenu);
    deinitMenuLogbookRenderer(grove->rndr);
    freeFont(&grove->menuFont);
    free(grove);
}

static void cGroveMainLoop(int64_t elapsedUs) 
{
    // Main Menu
    buttonEvt_t evt = {0};
    while(checkButtonQueueWrapper(&evt))
    {
        grove->cGroveMenu = menuButton(grove->cGroveMenu, evt);
    }
    drawMenuLogbook(grove->cGroveMenu, grove->rndr, 0);
}

static void cGroveMenuCB(const char* label, bool selected, uint32_t settingVal)
{
    if (selected)
    {
        if (label == cGroveStrPlay) {
            // TODO: Load Grove
        } else if (label == cGroveCompStrFight) {
            // TODO: Load Fight
        } else if (label == cGroveCompStrRace) {
            // TODO: Load Race
        } else if (label == cGroveCompStrPerf) {
            // TODO: Load Performance
        } else if (label == cGroveShopStrGroc) {
            // TODO: Load Store with grocery
        } else if (label == cGroveShopStrToy) {
            // TODO: Load Store with toys
        } else if (label == cGroveShopStrBook) {
            // TODO: Load Store with books
        } else if (label == cGroveShopStrSchool) {
            // TODO: Load School
        } else if (label == cGroveUser) {
            // TODO: Load Text entry
        } else if (label == cGrovePronounHe) {
            grove->player.pronouns = HE_HIM;
        } else if (label == cGrovePronounShe) {
            grove->player.pronouns = SHE_HER;
        } else if (label == cGrovePronounThey) {
            grove->player.pronouns = THEY_THEM;
        } else if (label == cGrovePronounOther) {
            grove->player.pronouns = OTHER;
        } else if (label == cGroveOLStrEnable) {
            cGroveToggleOnlineFeatures();
        }
    }
}

// Graphics Functions
//==============================================================================
static void cGroveBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    
}

// Wireless functions
//==============================================================================
static void cGroveToggleOnlineFeatures(){
    grove->online = !grove->online;
    // TODO: Turn on and off online features
}

static void cGroveEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{

}

static void cGroveEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{

}
