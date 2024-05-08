#ifndef SOKO_GAMERULES_H
#define SOKO_GAMERULES_H

/// @brief call [entity][tile] to get a bool that is true if that entity can NOT walk (or get pushed onto) that tile.
// bool sokoEntityTileCollision[4][8];

sokoTile_t sokoGetTile(int, int);
void sokoConfigGamemode(soko_abs_t* gamestate, soko_var_t variant);

// utility/shared functions.
void sharedGameLoop(soko_abs_t* self);
sokoDirection_t sokoDirectionFromDelta(int, int);

// entity pushing.
void sokoTryPlayerMovement(void);
bool sokoTryMoveEntityInDirection(sokoEntity_t*, int, int, uint16_t);

// classic and default
void absSokoGameLoop(soko_abs_t* self, int64_t elapsedUs);
void absSokoTryPlayerMovement(soko_abs_t* self);
bool absSokoTryMoveEntityInDirection(soko_abs_t* self, sokoEntity_t* entity, int dx, int dy, uint16_t push);
void absSokoDrawTiles(soko_abs_t* self, sokoLevel_t* level);
bool absSokoAllCratesOnGoal(soko_abs_t* self);
sokoTile_t absSokoGetTile(soko_abs_t* self, int x, int y);
bool allCratesOnGoal(void);

// euler
void eulerSokoTryPlayerMovement(soko_abs_t* self);
bool eulerNoUnwalkedFloors(soko_abs_t* self);

//lasers
sokoCollision_t sokoBeamImpact(soko_abs_t* self, sokoEntity_t* emitter);
int sokoBeamImpactRecursive(soko_abs_t* self, int emitter_x, int emitter_y, sokoDirection_t emitterDir,sokoEntity_t* rootEmitter);
sokoDirection_t sokoRedirectDir(sokoDirection_t emitterDir, bool inverted);
bool sokoLaserEntityCollision(sokoEntityType_t testEntity);
bool sokoLaserTileCollision(sokoTile_t testTile);
void laserBounceSokoGameLoop(soko_abs_t* self, int64_t elapsedUs);
sokoVec_t sokoGridToPix(soko_abs_t* self, sokoVec_t grid);
void drawLaserFromEntity(soko_abs_t* self, sokoEntity_t* emitter);
sokoVec_t sokoAddCoord(sokoVec_t op1, sokoVec_t op2);

// overworld
void overworldSokoGameLoop(soko_abs_t* self, int64_t elapsedUs);
bool overworldPortalEntered(soko_abs_t* self);
void restartCurrentLevel(soko_abs_t* self);
void exitToOverworld(soko_abs_t* self);

#endif // SOKO_GAMERULES_H