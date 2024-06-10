#pragma once

#include <swadge2024.h>

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    TUI_MENU,
    TUI_CONNECTING,
    TUI_GAME,
    TUI_PIECE_SELECT,
    TUI_HOW_TO,
} tttUi_t;

typedef enum __attribute__((packed))
{
    TGS_NOT_PLAYING,
    TGS_PLACING_PIECE,
    TGS_WAITING,
} tttGameState_t;

typedef enum __attribute__((packed))
{
    NO_CURSOR,
    SELECT_SUBGAME,
    SELECT_CELL,
    SELECT_CELL_LOCKED,
} tttCursorMode_t;

typedef enum __attribute__((packed))
{
    TTT_PIECE_X,
    TTT_PIECE_O,
} tttPiece_t;

typedef enum __attribute__((packed))
{
    TTT_NONE,
    TTT_P1,
    TTT_P2,
} tttPlayer_t;

typedef enum __attribute__((packed))
{
    MSG_SELECT_PIECE,
    MSG_MOVE_CURSOR,
    MSG_PLACE_PIECE,
} tttMsgType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    tttPlayer_t game[3][3];
    tttPlayer_t winner;
} tttSubgame_t;

typedef struct
{
    // Current UI being shown
    tttUi_t ui;
    // Main Menu
    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;
    font_t font_righteous;
    font_t font_rodin;
    // Connection
    p2pInfo p2p;
    const char* conStr;
    // Gameplay
    tttGameState_t state;
    tttSubgame_t subgames[3][3];
    vec_t cursor;
    vec_t selectedSubgame;
    tttCursorMode_t cursorMode;
    tttPiece_t p1Piece;
    tttPiece_t p2Piece;
    // Assets
    wsg_t piece_x_big;
    wsg_t piece_x_small;
    wsg_t piece_o_big;
    wsg_t piece_o_small;
} ultimateTTT_t;

typedef struct
{
    tttMsgType_t type;
    tttPiece_t piece;
} tttMsgSelectPiece_t;

typedef struct
{
    tttMsgType_t type;
    tttCursorMode_t cursorMode;
    vec_t selectedSubgame;
    vec_t cursor;
} tttMsgMoveCursor_t;

typedef struct
{
    tttMsgType_t type;
    vec_t selectedSubgame;
    vec_t selectedCell;
} tttMsgPlacePiece_t;

void tttMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

extern swadgeMode_t tttMode;