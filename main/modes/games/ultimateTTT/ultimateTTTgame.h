#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "ultimateTTT.h"

//==============================================================================
// Defines
//==============================================================================

#define CHECKER_COLOR_1 c000
#define CHECKER_COLOR_2 c111

//==============================================================================
// Function Declarations
//==============================================================================

void tttHandleGameInput(ultimateTTT_t* ttt, buttonEvt_t* evt);
void tttDrawGame(ultimateTTT_t* ttt, uint32_t elapsedUs);
void tttDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t m, paletteColor_t color);

void tttBeginGame(ultimateTTT_t* ttt);

void tttSendMarker(ultimateTTT_t* ttt, int32_t markerIdx);
void tttReceiveMarker(ultimateTTT_t* ttt, const tttMsgSelectMarker_t* rxSel);
void tttSendCursor(ultimateTTT_t* ttt);
void tttReceiveCursor(ultimateTTT_t* ttt, const tttMsgMoveCursor_t* msg);
void tttSendPlacedMarker(ultimateTTT_t* ttt);
void tttReceivePlacedMarker(ultimateTTT_t* ttt, const tttMsgPlaceMarker_t* msg);

bool tttCursorIsValid(ultimateTTT_t* ttt, const vec_t* cursor);
tttPlayer_t tttCheckWinner(const tttPlayer_t subgame[3][3]);