#ifndef _LUMBERJACK_PLAYER_H_
#define _LUMBERJACK_PLAYER_H_

#include "swadge2024.h"


//Let's start making a mess right away
typedef struct 
{
    wsg_t frames[21];
    bool flipped;
    int state;
    int currentFrame;
    int x;
    int y;
    int animationSpeed;
    int64_t timerFrameUpdate;
} lumberjackHero_t;

void lumberjackSetupPlayer(lumberjackHero_t* hero, int character);
void lumberjackSpawnPlayer(lumberjackHero_t* hero, int x, int y, int facing);
int lumberjackGetPlayerAnimation(lumberjackHero_t* hero);

#endif