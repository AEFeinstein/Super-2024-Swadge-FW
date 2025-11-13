#pragma once
#include "swadge2024.h"
#include "dn_typedef.h"
#include "dn_entityManager.h"

//==============================================================================
// Defines
//==============================================================================

#define DN_NUM_TROUPE_PERMUTATIONS 7

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    UI_MENU,
    UI_CONNECTING,
    UI_GAME,
    UI_RESULT,
} dn_Ui_t;

typedef enum __attribute((packed))
{
    DN_RESULT_WIN,
    DN_RESULT_LOSE,
    DN_RESULT_DRAW,
    DN_RESULT_DISCONNECT,
    DN_RESULT_RECORDS,
} dn_Result_t;

typedef enum __attribute__((packed))
{
    TDIFF_EASY,
    TDIFF_MEDIUM,
    TDIFF_HARD,
} dn_CpuDifficulty_t;

//==============================================================================
// Structs
//==============================================================================
typedef struct
{
    dn_CpuDifficulty_t difficulty;
    int64_t delayTime;
} dn_CpuData_t;

typedef struct dn_gameData_t
{
    p2pInfo p2p;
    // Current UI being shown
    dn_Ui_t ui;
    // Main Menu
    menu_t* menu;
    menu_t* bgMenu;
    menuMegaRenderer_t* menuRenderer;
    font_t font_ibm; // IBM VGA 8 font
    font_t font_righteous;
    font_t outline_righteous;
    // All buttons states
    uint16_t btnState;
    // Momentary downpresses on each button
    uint16_t btnDownState;
    rectangle_t camera;
    // font_t font_righteous;
    // Connection
    const char** conStrs;
    int16_t numConStrs;
    bool singleSystem;
    bool passAndPlay;
    int32_t elapsedUs; // Time elapsed since the last frame in microseconds
    dn_entityManager_t entityManager;
    uint16_t generalTimer;
    dn_phase_t phase;                   // The current phase of the game
    dn_characterSet_t characterSets[2]; // character sets of p1 and p2.
    dn_CpuData_t cpu;
    dn_Result_t lastResult;
    dn_asset_t assets[NUM_ASSETS];

    uint8_t alphaFaceDir; // 0 = down, 1 = left, 2 = up, 3 = right
    bool isPlayer1;       // True if the player on this device is P1. False if P2
    char playerNames[2][32];
    char shortPlayerNames[2][9];
    int8_t rerolls[2];   // The amount of rerolls for p1 and p2.
    bool resolvingRemix; // True if a unit is still needing to do a second action.
    bool pawnPlunging;
    dn_boardPos_t selectorPos;
    dn_entity_t* selectedUnit;
} dn_gameData_t;

//==============================================================================
// Functions
//==============================================================================

void dn_MsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);
void dn_ShowUi(dn_Ui_t ui);
void dn_setAssetMetaData(void);

//==============================================================================
// Externs
//==============================================================================

// extern const char tttName[];
// extern const char tttWinKey[];
// extern const char tttLossKey[];
// extern const char tttDrawKey[];
extern const char dnP1TroupeKey[];
extern const char dnP2TroupeKey[];
// extern const char tttTutorialKey[];
// extern const char tttUnlockKey[];
extern swadgeMode_t danceNetworkMode;
extern heatshrink_decoder* dn_hsd;
extern uint8_t* dn_decodeSpace;
// extern const int16_t markersUnlockedAtWins[NUM_UNLOCKABLE_MARKERS];
extern const trophyData_t danceNetworkTrophies[];