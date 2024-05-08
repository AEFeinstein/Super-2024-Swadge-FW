#pragma once

#include <swadge2024.h>

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    RGS_MENU,
    RGS_PLACING_PIECE,
    RGS_WAITING,
} ragGameState_t;

typedef enum __attribute__((packed))
{
    NO_CURSOR,
    SELECT_SUBGAME,
    SELECT_CELL,
    SELECT_CELL_LOCKED,
} ragCursorMode_t;

typedef enum __attribute__((packed))
{
    RAG_EMPTY,
    RAG_RING,
    RAG_GEM,
} ragPiece_t;

typedef enum __attribute__((packed))
{
    MSG_SELECT_PIECE,
    MSG_MOVE_CURSOR,
    MSG_PLACE_PIECE,
} ragMsgType_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    ragPiece_t game[3][3];
    ragPiece_t winner;
} ragSubgame_t;

typedef struct
{
    ragGameState_t state;
    ragSubgame_t subgames[3][3];
    vec_t cursor;
    vec_t selectedSubgame;
    ragCursorMode_t cursorMode;
    wsg_t piece_x_big;
    wsg_t piece_x_small;
    wsg_t piece_o_big;
    wsg_t piece_o_small;
    p2pInfo p2p;
    ragPiece_t p1Piece;
    ragPiece_t p2Piece;
} ringsAndGems_t;

typedef struct
{
    ragMsgType_t type;
    ragPiece_t piece;
} ragMsgSelectPiece_t;

typedef struct
{
    ragMsgType_t type;
    ragCursorMode_t cursorMode;
    vec_t selectedSubgame;
    vec_t cursor;
} ragMsgMoveCursor_t;

typedef struct
{
    ragMsgType_t type;
    vec_t selectedSubgame;
    vec_t selectedCell;
} ragMsgPlacePiece_t;

void ragMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

extern swadgeMode_t ragMode;