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
static void cGroveMainLoop(int64_t elapsedUs);
static void cGroveExitMode(void);
static void cGroveEnterMode(void);
static void cGroveBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void cGroveEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void cGroveEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

// Strings
//==============================================================================
static const char cGroveTitle[] = "Chowa Grove";
/* 
// User
static const char cGrovePronounHe[] = "He/Him";
static const char cGrovePronounShe[] = "She/Her";
static const char cGrovePronounThey[] = "They/them";

// NPC data
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

    // UI data
    uint8_t cursorPos;  // Position of the menu cursor

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

// Functions
//==============================================================================

static void cGroveEnterMode(void)
{
    grove = calloc(1, sizeof(cGrove_t));
}

static void cGroveExitMode(void)
{
    free(grove);
}

static void cGroveMainLoop(int64_t elapsedUs) 
{
    
}

static void cGroveBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{

}

static void cGroveEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{

}

static void cGroveEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{

}