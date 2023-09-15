#ifndef _LUMBERJACK_ENEMY_H_
#define _LUMBERJACK_ENEMY_H_

#include "swadge2024.h"

typedef struct
{
    bool flipped;
    bool onGround;
    bool active;

    bool flying;

    bool attackPressed;
    bool attackThisFrame;

    bool jumping;
    bool jumpPressed;
    bool jumpReady;
    int jumpTimer;

    int state;
    int currentFrame;
    int drawFrame;
    int x;
    int y;
    int spriteOffset;
    int vx;
    float vy;
    int maxVX;
    int type;
    int maxLevel;
    int respawn;

    int cX;
    int cY;
    int8_t cW;
    int8_t cH;

    bool upgrading;
    bool ready; // Ready to be placed because it's not in game
    bool showAlert;

    int width;
    int height;
    int tileHeight;

    int direction;
    int animationSpeed;
    int64_t timerFrameUpdate;
    char name[16];

} lumberjackEntity_t;

void lumberjackSetupEnemy(lumberjackEntity_t* enemy, int character);
uint8_t lumberjackGetEnemyAnimation(lumberjackEntity_t* enemy);
void lumberjackResetEnemy(lumberjackEntity_t* enemy);
void lumberjackRespawnEnemy(lumberjackEntity_t* enemy, int side);
bool checkCollision(lumberjackEntity_t* AA, lumberjackEntity_t* BB);
void lumberjackUpdateEnemy(lumberjackEntity_t* enemy, int newIndex);
void lumberjackDoEnemyControls(lumberjackEntity_t* enemy);

void lumberjackUpdateEnemyCollision(lumberjackEntity_t* enemy);
void lumberjackUpdatePlayerCollision(lumberjackEntity_t* player);

#endif