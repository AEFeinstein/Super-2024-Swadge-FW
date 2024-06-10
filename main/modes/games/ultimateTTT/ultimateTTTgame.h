#pragma once

#include "ultimateTTT.h"

void tttHandleGameInput(ultimateTTT_t* ttt, buttonEvt_t* evt);
void tttDrawGame(ultimateTTT_t* ttt);
void tttDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t m, paletteColor_t color);

void tttBeginGame(ultimateTTT_t* ttt);

void tttSendCursor(ultimateTTT_t* ttt);
void tttReceiveCursor(ultimateTTT_t* ttt, const tttMsgMoveCursor_t* msg);
void tttSendPlacedPiece(ultimateTTT_t* ttt);
void tttReceivePlacedPiece(ultimateTTT_t* ttt, const tttMsgPlacePiece_t* msg);
