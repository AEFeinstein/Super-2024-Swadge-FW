#ifndef SOKO_GAME_H
#define SOKO_GAME_H

#include "soko.h"

void sokoInitGame(soko_abs_t*);
void sokoInitGameBin(soko_abs_t*);
void sokoInitNewLevel(soko_abs_t* soko, soko_var_t variant);
void gameLoop(int64_t);
void drawTiles(sokoLevel_t*);

#endif // SOKO_GAME_H