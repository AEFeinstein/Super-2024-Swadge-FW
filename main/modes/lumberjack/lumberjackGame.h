#ifndef _LUMBERJACK_MODE_H
#define _LUMBERJACK_MODE_H_


void lumberjackStartGameMode(lumberjackGameType_t type);
void lumberjackSetupLevel(int index);
void lumberjackDoControls(void);
static void lumberjackTileMap(void);
static void lumberjackUpdate(int64_t elapseUs);

void lumberjackGameLoop(int64_t elapsedUs);

static void restartLevel();

static void lumberjackUpdateEntity(lumberjackEntity_t* entity, int64_t elapsedUs);

static lumberjackTile_t* lumberjackGetTile(int x, int y);
static bool lumberjackIsCollisionTile(int index);

void lumberjackGameDebugLoop(int64_t elapsedUs);

void lumberjackDetectBump(lumberjackTile_t* tile);
void lumberjackSpawnCheck(int64_t elapseUs);

void baseMode(int64_t elapsedUs);

#endif