#pragma once

#include <swadge2024.h>

//==============================================================================
// Defines
//==============================================================================

#define NUM_UNLOCKABLE_MARKERS 13

#define ARROW_BLINK_PERIOD 1000000

//==============================================================================
// Enums
//==============================================================================

typedef enum __attribute__((packed))
{
    TUI_MENU,
    TUI_CONNECTING,
    TUI_GAME,
    TUI_MARKER_SELECT,
    TUI_HOW_TO,
    TUI_RESULT,
} tttUi_t;

typedef enum __attribute__((packed))
{
    TGS_NOT_PLAYING,
    TGS_PLACING_MARKER,
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
    MSG_SELECT_MARKER,
    MSG_MOVE_CURSOR,
    MSG_PLACE_MARKER,
} tttMsgType_t;

typedef enum __attribute((packed))
{
    TTR_WIN,
    TTR_LOSE,
    TTR_DRAW,
    TTR_DISCONNECT,
    TTR_RECORDS,
} tttResult_t;

typedef enum __attribute__((packed))
{
    TCPU_INACTIVE,
    TCPU_THINKING,
    TCPU_MOVING,
} tttCpuState_t;

typedef enum __attribute__((packed))
{
    TDIFF_EASY,
    TDIFF_MEDIUM,
    TDIFF_HARD,
} tttCpuDifficulty_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsg_t small;
    wsg_t large;
} tttMarkerSizeAssets_t;

typedef struct
{
    tttMarkerSizeAssets_t red;
    tttMarkerSizeAssets_t blue;
} tttMarkerColorAssets_t;

typedef struct
{
    tttPlayer_t game[3][3];
    tttPlayer_t winner;
} tttSubgame_t;

typedef struct
{
    tttCpuState_t state;
    tttCpuDifficulty_t difficulty;
    vec_t destSubgame;
    vec_t destCell;
    int64_t delayTime;
} tttCpuData_t;

typedef struct
{
    bool singlePlayer;
    playOrder_t singlePlayerPlayOrder;
    p2pInfo p2p;
    tttGameState_t state;
    tttSubgame_t subgames[3][3];
    vec_t cursor;
    vec_t selectedSubgame;
    tttCursorMode_t cursorMode;
    vec_t cursorLastDir;
    int32_t p1MarkerIdx;
    int32_t p2MarkerIdx;
    tttCpuData_t cpu;
} tttGameData_t;

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
    const char** conStrs;
    int16_t numConStrs;
    // Gameplay
    tttGameData_t game;
    int16_t gameSize;
    int16_t cellSize;
    int16_t subgameSize;
    vec_t gameOffset;
    // Assets
    tttMarkerColorAssets_t markerWsg[NUM_UNLOCKABLE_MARKERS];
    // For marker selection UI
    int32_t xSelectScrollTimer;
    int16_t xSelectScrollOffset;
    int32_t selectMarkerIdx;
    int32_t activeMarkerIdx;
    // Stats
    int32_t wins;
    int32_t losses;
    int32_t draws;
    tttResult_t lastResult;
    int32_t numUnlockedMarkers;
    // Instructions
    int32_t pageIdx;
    int32_t tutorialRead;
    int32_t arrowBlinkTimer;
    list_t instructionHistory;
    bool showingInstructions;
    arrow_t instructionArrow;
} ultimateTTT_t;

typedef struct
{
    tttMsgType_t type;
    int32_t markerIdx;
} tttMsgSelectMarker_t;

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
} tttMsgPlaceMarker_t;

//==============================================================================
// Functions
//==============================================================================

void tttMsgTxCbFn(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);
void tttShowUi(tttUi_t ui);

//==============================================================================
// Externs
//==============================================================================

extern const char tttWinKey[];
extern const char tttLossKey[];
extern const char tttDrawKey[];
extern const char tttMarkerKey[];
extern const char tttTutorialKey[];
extern const char tttUnlockKey[];
extern swadgeMode_t tttMode;
extern const int16_t markersUnlockedAtWins[NUM_UNLOCKABLE_MARKERS];
