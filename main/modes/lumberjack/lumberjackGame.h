#ifndef _LUMBERJACK_MODE_H
    #define _LUMBERJACK_MODE_H_

void lumberjackStartGameMode(lumberjack_t* main, uint8_t characterIndex);

void lumberjackExitGameMode(void);
void lumberjackSetupLevel(int index);
void lumberjackUnloadLevel(void);
void lumberjackDoControls(void);
void lumberjackTileMap(void);
void lumberjackDrawWaterLevel(void);
void lumberjackUpdate(int64_t elapseUs);

void lumberjackTitleLoop(int64_t elapsedUs);
void lumberjackGameLoop(int64_t elapsedUs);
void lumberjackUpdateLocation(int ghostX, int ghostY, int frame);

void restartLevel(void);

void lumberjackGameDebugLoop(int64_t elapsedUs);

void lumberjackDetectBump(lumberjackTile_t* tile);
void lumberjackSpawnCheck(int64_t elapseUs);

void lumberjackScoreDisplay(int score, int locationX);

void baseMode(int64_t elapsedUs);
void lumberjackSendAttack(int number);

#endif