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

void restartLevel(void);

void lumberjackGameDebugLoop(int64_t elapsedUs);

void lumberjackDetectBump(lumberjackTile_t* tile);
bool lumberjackSpawnCheck(int64_t elapseUs);
void lumberjackAttackCheck(int64_t elapseUs);

void lumberjackScoreDisplay(int score, int locationX);
void lumberjackTitleDisplayText(char* string, int locationX, int locationY);

void baseMode(int64_t elapsedUs);

void lumberjackQualityCheck(void);

void lumberjackSendAttack(uint8_t* number);
void lumberjackSendScore(int score);
void lumberjackSendDeath(bool gameover);
void lumberjackSendBump(void);

void lumberjackOnReceiveAttack(const uint8_t* attack, int len);
void lumberjackOnReceiveScore(const uint8_t* score);
void lumberjackOnReceiveDeath(bool gameover);
void lumberjackOnReceiveBump(void);

void lumberjackGameReady(void);
void lumberjackPlayGame(void);
void lumberjackSendGo(void);
void drawSolidWsg(const wsg_t* wsg, int16_t xOff, int16_t yOff, bool flipLR, bool flipUD, uint8_t inColor);


void lumberjackUseBlock(void);
void lumberjackSaveSave(void);


#endif