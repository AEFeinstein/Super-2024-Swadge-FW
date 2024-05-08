#pragma once

#include "ringsAndGems.h"

void ragHandleGameInput(ringsAndGems_t* rag, buttonEvt_t* evt);
void ragDrawGame(ringsAndGems_t* rag);
void ragDrawGrid(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t m, paletteColor_t color);

void ragBeginGame(ringsAndGems_t* rag);

void ragSendCursor(ringsAndGems_t* rag);
void ragReceiveCursor(ringsAndGems_t* rag, const ragMsgMoveCursor_t* msg);
void ragSendPlacedPiece(ringsAndGems_t* rag);
void ragReceivePlacedPiece(ringsAndGems_t* rag, const ragMsgPlacePiece_t* msg);
