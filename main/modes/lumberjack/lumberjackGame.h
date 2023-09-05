#ifndef _LUMBERJACK_MODE_H
    #define _LUMBERJACK_MODE_H_

void lumberjackStartGameMode(lumberjackGameType_t type);
void lumberjackExitGameMode(void);
void lumberjackSetupLevel(int index);
void lumberjackDoControls(void);
void lumberjackTileMap(void);
void lumberjackUpdate(int64_t elapseUs);

void lumberjackGameLoop(int64_t elapsedUs);

void restartLevel(void);

void lumberjackGameDebugLoop(int64_t elapsedUs);

void lumberjackDetectBump(lumberjackTile_t* tile);
void lumberjackSpawnCheck(int64_t elapseUs);

void baseMode(int64_t elapsedUs);

#endif