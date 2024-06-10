#pragma once

#include <swadge2024.h>

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    TGS_MENU,
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
    TTT_EMPTY,
    TTT_RING,
    TTT_GEM,
} tttPiece_t;

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
    tttPiece_t game[3][3];
    tttPiece_t winner;
} tttSubgame_t;

typedef struct
{
    menu_t* menu;
    menuManiaRenderer_t* menuRenderer;
    font_t font_righteous;
    font_t font_rodin;
    tttGameState_t state;
    tttSubgame_t subgames[3][3];
    vec_t cursor;
    vec_t selectedSubgame;
    tttCursorMode_t cursorMode;
    wsg_t piece_x_big;
    wsg_t piece_x_small;
    wsg_t piece_o_big;
    wsg_t piece_o_small;
    p2pInfo p2p;
    tttPiece_t p1Piece;
    tttPiece_t p2Piece;
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