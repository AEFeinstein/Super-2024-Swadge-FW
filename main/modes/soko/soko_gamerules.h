#ifndef SOKO_GAMERULES_H
#define SOKO_GAMERULES_H

void sokoTryPlayerMovement(void);
sokoTile_t sokoGetTile(int, int);
bool sokoTryMoveEntityInDirection(sokoEntity_t*, int, int,uint16_t);
bool allCratesOnGoal(void);

void absSokoGameLoop( soko_abs_t *self, int64_t elapsedUs);
void absSokoTryPlayerMovement(soko_abs_t *self);
bool absSokoTryMoveEntityInDirection(soko_abs_t *self, sokoEntity_t* entity, int dx, int dy, uint16_t push);
void absSokoDrawTiles(soko_abs_t *self, sokoLevel_t* level);
bool absSokoAllCratesOnGoal(soko_abs_t *self);
sokoTile_t absSokoGetTile(soko_abs_t *self, int x, int y);

sokoDirection_t sokoDirectionFromDelta(int, int);

void sokoConfigGamemode(soko_abs_t* gamestate, soko_var_t variant);

#endif //SOKO_GAMERULES_H