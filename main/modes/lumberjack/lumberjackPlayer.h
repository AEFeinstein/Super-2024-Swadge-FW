#ifndef _LUMBERJACK_PLAYER_H_
#define _LUMBERJACK_PLAYER_H_

#include "lumberjack_types.h"

void lumberjackSetupPlayer(lumberjackEntity_t* hero, int character);
void lumberjackSpawnPlayer(lumberjackEntity_t* hero, int x, int y, int facing);
void lumberjackRespawn(lumberjackEntity_t* hero);
 int lumberjackGetPlayerAnimation(lumberjackEntity_t* hero);

#endif 