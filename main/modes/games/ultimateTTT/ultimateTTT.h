#pragma once

#include <swadge2024.h>

//==============================================================================
// Defines
//==============================================================================

#define NUM_UNLOCKABLE_PIECES 4

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
    TUI_RESULT,
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
    TTT_NONE,
    TTT_P1,
    TTT_P2,
    TTT_DRAW,
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
    wsg_t small;
    wsg_t large;
} tttPieceSizeAssets_t;

typedef struct
{
    tttPieceSizeAssets_t red;
    tttPieceSizeAssets_t blue;
} tttPieceColorAssets_t;

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
    menu_t* bgMenu;
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
    int32_t p1PieceIdx;
    int32_t p2PieceIdx;
    // Assets
    tttPieceColorAssets_t pieceWsg[NUM_UNLOCKABLE_PIECES];
    // For piece selection UI
    int32_t xSelectScrollTimer;
    int16_t xSelectScrollOffset;
    int32_t selectPieceIdx;
    int32_t activePieceIdx;
    wsg_t selectArrow;
    // Stats
    int32_t wins;
    int32_t losses;
    int32_t draws;
    tttPlayer_t lastResult;
} ultimateTTT_t;

typedef struct
{
    tttMsgType_t type;
    int32_t pieceIdx;
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

//==============================================================================
// Functions
//==============================================================================

void tttMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);

//==============================================================================
// Externs
//==============================================================================

extern const char tttWinKey[];
extern const char tttLossKey[];
extern const char tttDrawKey[];
extern const char tttPieceKey[];
extern swadgeMode_t tttMode;