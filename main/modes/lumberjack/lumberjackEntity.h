#ifndef _LUMBERJACK_ENEMY_H_
#define _LUMBERJACK_ENEMY_H_

#include "swadge2024.h"


typedef struct
{
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
    int spriteOffset;
    float vx;
    float vy;
    float maxVX;
    int type;
    int maxLevel;
    int respawn;

    bool upgrading;
    bool ready; //Ready to be placed because it's not in game
    bool showAlert;

    int width;
    int height;
    int tileHeight;

    int direction;
    int animationSpeed;
    int64_t timerFrameUpdate;
} lumberjackEntity_t;

void lumberjackSetupEnemy(lumberjackEntity_t* enemy, int character);
uint8_t lumberjackGetEnemyAnimation(lumberjackEntity_t* enemy);
void lumberjackResetEnemy(lumberjackEntity_t* enemy);
void lumberjackRespawnEnemy(lumberjackEntity_t* enemy, int side);
bool checkCollision(lumberjackEntity_t* AA, lumberjackEntity_t* BB);
void lumberjackUpdateEnemy(lumberjackEntity_t* enemy, int newIndex);
void lumberjackDoEnemyControls(lumberjackEntity_t* enemy);

#endif