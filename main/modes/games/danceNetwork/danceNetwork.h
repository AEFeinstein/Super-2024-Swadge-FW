#pragma once
#include "swadge2024.h"
#include "dn_typedef.h"

extern swadgeMode_t danceNetworkMode;

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

typedef struct{
    wsg_t groundTile;
    wsg_t alphaDown; //face down
    wsg_t alphaUp; //face up
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