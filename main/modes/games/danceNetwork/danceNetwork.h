#pragma once
#include "swadge2024.h"
#include "dn_typedef.h"

//==============================================================================
// Defines
//==============================================================================

#define NUM_CHARACTERS 2

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

typedef enum __attribute__((packed))
{
    DN_ALPHA,
    DN_CHESS
} dn_Characters;

typedef enum __attribute__((packed))
{
    DN_UP,
    DN_DOWN
} dn_facingDirs;

typedef enum __attribute__((packed))
{
    DN_PAWN,
    DN_KING
} dn_unitRank;


//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsg_t sprite;
    int8_t xOff; //an offset from the top left of the tile graphic to the top left of this sprite.
    int8_t yOff;
} dn_CharacterAsset_t;

typedef struct{
    wsg_t groundTile;
} dn_sprites_t;



typedef struct
{
    dn_CpuDifficulty_t difficulty;
    int64_t delayTime;
} dn_CpuData_t;

typedef struct{
    int8_t x;
    int8_t y;
} dn_boardPos;

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
    dn_CharacterAsset_t characterAssets[NUM_CHARACTERS][2][2];//character idx, pawn/king, up/down
    uint8_t generalTimer;
    // For marker selection UI
    int32_t xSelectScrollTimer;
    int16_t xSelectScrollOffset;
    int8_t selectCharacterIdx;
    int8_t characterIndices[2];//character indices of p1 and p2. p1 is [0].
    dn_CpuData_t cpu;
    dn_Result_t lastResult;
    dn_sprites_t sprites;
    
    uint8_t selection[2];//x and y indices of the selected tile
    uint8_t alphaFaceDir; //0 = down, 1 = left, 2 = up, 3 = right
    wsgPalette_t redFloor1;
    bool isPlayer1;//True if the player on this device is P1. False if P2
    dn_boardPos UnitPositions[2][5];//two players each with 5 units. Unit at [][2] is king.
} dn_gameData_t;

//==============================================================================
// Functions
//==============================================================================

void dn_MsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);
void dn_ShowUi(dn_Ui_t ui);
void dn_InitializeGame();
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