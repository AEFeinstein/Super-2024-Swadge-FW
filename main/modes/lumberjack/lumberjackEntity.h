#ifndef _LUMBERJACK_ENEMY_H_
#define _LUMBERJACK_ENEMY_H_

#include "swadge2024.h"


typedef struct
{
    wsg_t frames[21];
    bool flipped;
    bool onGround;    
    bool active;

    bool flying;

    bool jumping;
    bool jumpPressed;
    bool jumpReady;
    int jumpTimer;

    int state;
    int currentFrame;
    int x;
    int y;
    float vx;
    float vy;
    float maxVX;
    int type;
    int respawn;

    int width;
    int height;
    int tileHeight;

    int tx;
    int ty;
    int direction;
    int animationSpeed;
    int64_t timerFrameUpdate;
} lumberjackEntity_t;

void lumberjackSetupEnemy(lumberjackEntity_t* enemy, int character);
void lumberjackUnloadEnemy(lumberjackEntity_t* enemy);
uint8_t lumberjackGetEnemyAnimation(lumberjackEntity_t* enemy);

#endif