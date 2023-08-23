#ifndef SOKO_GAMERULES_H
#define SOKO_GAMERULES_H

/// @brief call [entity][tile] to get a bool that is true if that entity can NOT walk (or get pushed onto) that tile.
//bool sokoEntityTileCollision[4][8];

void sokoTryPlayerMovement(void);
sokoTile_t sokoGetTile(int, int);
bool sokoTryMoveEntityInDirection(sokoEntity_t*, int, int,uint16_t);
bool allCratesOnGoal(void);

//classic and default
void absSokoGameLoop( soko_abs_t *self, int64_t elapsedUs);
void absSokoTryPlayerMovement(soko_abs_t *self);
bool absSokoTryMoveEntityInDirection(soko_abs_t *self, sokoEntity_t* entity, int dx, int dy, uint16_t push);
void absSokoDrawTiles(soko_abs_t *self, sokoLevel_t* level);
bool absSokoAllCratesOnGoal(soko_abs_t *self);
sokoTile_t absSokoGetTile(soko_abs_t *self, int x, int y);

sokoDirection_t sokoDirectionFromDelta(int, int);

void sokoConfigGamemode(soko_abs_t* gamestate, soko_var_t variant);

sokoCollision_t sokoBeamImpact(soko_abs_t* self, sokoEntity_t* emitter);
sokoVec_t sokoAddCoord(sokoVec_t op1, sokoVec_t op2);

//euler
void eulerSokoTryPlayerMovement(soko_abs_t *self);
bool eulerNoUnwalkedFloors(soko_abs_t *self);

#endif //SOKO_GAMERULES_H