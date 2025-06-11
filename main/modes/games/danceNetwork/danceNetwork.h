#pragma once
#include "swadge2024.h"
#include "dn_typedef.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_CHARACTERS 3

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    UI_MENU,
    UI_CONNECTING,
    UI_GAME,
    UI_CHARACTER_SELECT,
    UI_HOW_TO,
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
    wsg_t kingUp;
    wsg_t kingDown;
    wsg_t pawnUp;
    wsg_t pawnDown;
} dn_CharacterAssets_t;

typedef struct{
    wsg_t groundTile;
} dn_sprites_t;

typedef struct{
    uint16_t yOffset;
    int16_t yVel;
} dn_tileData_t;

typedef struct
{
    dn_CpuDifficulty_t difficulty;
    int64_t delayTime;
} dn_CpuData_t;

typedef struct {
    p2pInfo p2p;
    // Current UI being shown
    dn_Ui_t ui;
    // Main Menu
    menu_t* menu;
    menu_t* bgMenu;
    menuManiaRenderer_t* menuRenderer;
    font_t font_rodin;
    // font_t font_righteous;
    // Connection
    const char** conStrs;
    int16_t numConStrs;
    bool singleSystem;
    bool passAndPlay;
    // Assets
    dn_CharacterAssets_t characterAssets[NUM_CHARACTERS];
    uint8_t generalTimer;
    // For marker selection UI
    int32_t xSelectScrollTimer;
    int16_t xSelectScrollOffset;
    int8_t selectMarkerIdx;
    int32_t activeMarkerIdx;
    dn_CpuData_t cpu;
    dn_Result_t lastResult;
    dn_sprites_t sprites;
    dn_tileData_t tiles[BOARD_SIZE][BOARD_SIZE];
    uint8_t selection[2];//x and y indices of the selected tile
    uint8_t alphaFaceDir; //0 = down, 1 = left, 2 = up, 3 = right
} dn_gameData_t;

//==============================================================================
// Functions
//==============================================================================

void dn_MsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);
void dn_ShowUi(dn_Ui_t ui);
void dn_InitializeCharacterSelect();


//==============================================================================
// Externs
//==============================================================================

// extern const char tttName[];
// extern const char tttWinKey[];
// extern const char tttLossKey[];
// extern const char tttDrawKey[];
extern const char dnCharacterKey[];
// extern const char tttTutorialKey[];
// extern const char tttUnlockKey[];
extern swadgeMode_t danceNetworkMode;
// extern const int16_t markersUnlockedAtWins[NUM_UNLOCKABLE_MARKERS];