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
    uint16_t buttonState;   // State of the buttons
    bool online;            // True is online, False is offline

    // State tracking
    State_t currState;          // Current game state
    int8_t customMenuSelect;    // Current selection on custom menu

    // Swadge structs
    menu_t* cGroveMenu;             // Main menu for cGrove
    menuLogbookRenderer_t* rndr;    // Logbook renderer

    // Assets
    font_t menuFont;    // Menu font FIXME: Change to new sonic font

    // Chowa Grove data
    playerProfile_t player;                     // Player struct
    playerProfile_t guests[MAX_PREV_GUESTS];    // List of guest players
    Cosmetics_t cosmeticInv[255];               // List of owned cosmetics
} cGrove_t;

// Function Prototypes
//==============================================================================
static void cGroveMainLoop(int64_t);
static void cGroveExitMode(void);
static void cGroveEnterMode(void);
static void cGroveInitMenu(void);
static void cGroveMenuCB(const char*, bool, uint32_t);
static void cGroveBackgroundDrawCallback(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t);
static void cGroveProfileMain(buttonEvt_t evt);
static void cGroveShowMainProfile(void);
static void cGroveShowSubProfile(buttonEvt_t evt);
static void cGroveToggleOnlineFeatures(void);
static void cGroveEspNowRecvCb(const esp_now_recv_info_t*, const uint8_t*, uint8_t, int8_t);
static void cGroveEspNowSendCb(const uint8_t*, esp_now_send_status_t);

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
    cGroveInitMenu();

    // Initialize system
    grove->currState = MENU;

    // TEST CODE
    static char test[] = "test ";
    static char nameBuffer[USERNAME_CHARS];
    strcpy(grove->player.username, test);
    grove->player.pronouns = HE_HIM;
    grove->player.mood = HAPPY;
    for (int i = 0; i < MAX_PREV_GUESTS; i++){
        snprintf(nameBuffer, sizeof(nameBuffer)-1, "Test_%" PRIu32, i);
        strcpy(grove->guests[i].username, nameBuffer);
        grove->guests[i].pronouns = HE_HIM;
        grove->guests[i].mood = HAPPY;
    }
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
    buttonEvt_t evt = {0};
    switch (grove->currState){
        case MENU:
            while(checkButtonQueueWrapper(&evt))
            {
                grove->cGroveMenu = menuButton(grove->cGroveMenu, evt);
            }
            drawMenuLogbook(grove->cGroveMenu, grove->rndr, 0);
            break;
        case PROFILE:
            cGroveProfileMain(evt);
            break;
        case SUBPROFILE:
            cGroveShowSubProfile(evt);
            break;
        default:
            // TODO: Default
    }
}

// Menus
// =============================================================================
static void cGroveInitMenu()
{
    grove->cGroveMenu = initMenu(cGroveTitle, cGroveMenuCB);
    addSingleItemToMenu(grove->cGroveMenu, cGroveStrPlay);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveStrCompete);
    addSingleItemToMenu(grove->cGroveMenu, cGroveCompStrFight);
    addSingleItemToMenu(grove->cGroveMenu, cGroveCompStrRace);
    addSingleItemToMenu(grove->cGroveMenu, cGroveCompStrPerf);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveStrShops);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrGroc);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrToy);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrBook);
    addSingleItemToMenu(grove->cGroveMenu, cGroveShopStrSchool);
    grove->cGroveMenu = endSubMenu(grove->cGroveMenu);
    grove->cGroveMenu = startSubMenu(grove->cGroveMenu, cGroveStrOnline);
    addSingleItemToMenu(grove->cGroveMenu, cGroveOLStrViewProf);
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
        } else if (label == cGroveOLStrViewProf) {
            grove->customMenuSelect = 0;
            grove->currState = PROFILE;
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

static int8_t cGroveKeepBounds(buttonEvt_t evt, int8_t maxBound, int8_t *selection)
{
    // Increment
    if (evt.state & PB_UP) {
        *selection -= 1;
    } else {
        *selection += 1;
    }
    // Reset if out of bounds
    if (*selection >= maxBound) {
        *selection = 0;
    } else if (*selection < 0) {
        *selection = maxBound - 1;
    }
    return *selection;
}

// Online
// =============================================================================
static void cGroveProfileMain(buttonEvt_t evt){
    while(checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            if ((evt.state & PB_UP) || (evt.state & PB_DOWN)){  // Switch profiles
                cGroveKeepBounds(evt, MAX_PREV_GUESTS + 1, &grove->customMenuSelect);
            } else if (evt.state & PB_A) {                      // Select profile to view in more detail
                grove->currState = SUBPROFILE;
            } else {
                grove->currState = MENU;
            }
        }
        cGroveShowMainProfile();
    }
}

static void cGroveShowMainProfile()
{
    // Profile background
    fillDisplayArea(0, 0, H_SCREEN_SIZE, V_SCREEN_SIZE, c111); // TEMP

    // Profile 
    playerProfile_t ply;
    if (grove->customMenuSelect == 0) {
        ply = grove->player;
    } else {
        ply = grove->guests[(grove->customMenuSelect - 1)];
    }
    // Show username
    drawText(&grove->menuFont, c555, ply.username, 20, 20);
    // Show pronouns
    drawText(&grove->menuFont, c555, ply.username, 20, 50);
    // Show mood
    drawText(&grove->menuFont, c555, ply.username, 20, 80);
    // Show Chowa
    // - Name, color, mood, etc
    drawText(&grove->menuFont, c555, ply.username, 20, 110);

    // Display arrows to cycle through profiles
    // Grey out arrows at the top and bottom of list
}

static void cGroveShowSubProfile(buttonEvt_t evt)
{
    playerProfile_t ply;
    if (grove->customMenuSelect == 0) {
        ply = grove->player;
    } else {
        ply = grove->guests[(grove->customMenuSelect - 1)];
    }
    while(checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            grove->currState = PROFILE;
        }
        // Draw
        drawText(&grove->menuFont, c555, "GET PUNKED", 20, 140);
        drawText(&grove->menuFont, c555, ply.username, 20, 170);
    }
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

// Graphics Functions
//==============================================================================
static void cGroveBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    
}