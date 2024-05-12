#ifndef SOKO_UNDO_H
#define SOKO_UNDO_H

#include "swadge2024.h"
#include "soko.h"

//if isEntity is true, then x and y are the position to return the entity at entityindex to, rest is ignored.
//if isentity is false, then set the tile at position x,y to tile. rest is ignored.


#endif

void sokoHistoryTurnOver(soko_abs_t* soko);
void sokoAddTileMoveToHistory(soko_abs_t* soko, uint16_t tileX, uint16_t tileY, sokoTile_t oldTileType);
void sokoAddEntityMoveToHistory(soko_abs_t* soko, sokoEntity_t* entity, uint16_t oldX, uint16_t oldY, sokoDirection_t oldFacing);
void sokoUndo(soko_abs_t* soko);
void sokoInitHistory(soko_abs_t* soko);