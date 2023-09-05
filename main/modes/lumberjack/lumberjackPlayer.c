
#include "lumberjack_types.h"
#include "lumberjackEntity.h"
#include "lumberjackPlayer.h"

#include <esp_log.h>

void lumberjackSetupPlayer(lumberjackEntity_t* hero, int character)
{
    hero->height = 31;
    hero->width = 31;
    hero->tileHeight = 2;
    hero->maxVX = 15;
    hero->active = true;
    hero->showAlert = false;
    hero->upgrading = false;
    hero->spriteOffset = 0;
    hero->maxLevel = character;
    hero->type = character;

    if (character == 0)
    {
        hero->spriteOffset = 0;
    }
    else if (character == 1)
    {
        hero->spriteOffset = 17;

    } else 
    {
        hero->spriteOffset = 34;
    }
    
}

void lumberjackSpawnPlayer(lumberjackEntity_t* hero, int x, int y, int facing)
{
    hero->x = x;
    hero->y = 270;
    hero->vx = 0;
    hero->maxVX = 15;
    hero->vy = 0;
    hero->flipped = (facing == 0);
    hero->state = LUMBERJACK_IDLE;
    hero->timerFrameUpdate = 0;
    hero->active = true;
    hero->onGround = true;
    hero->ready = false;
}

void lumberjackRespawn(lumberjackEntity_t* hero)
{
    hero->x = 130;
    hero->maxVX = 15;
    hero->y = 270;
    hero->active = true;
    hero->ready = false;
    hero->vx = 0;
    hero->vy = 0;
    hero->flipped = 0;
    hero->state = LUMBERJACK_IDLE;
    hero->timerFrameUpdate = 0;
    hero->onGround = true;
    hero->maxLevel = 0;
}

int lumberjackGetPlayerAnimation(lumberjackEntity_t* hero)
{
    // int animationNone[] = {0};

    int animation =  hero->state;
    hero->animationSpeed = 150000;

    if (hero->onGround == false && hero->jumping == false && hero->active)
    {
        const int animationFall[] = {13};
        return animationFall[hero->currentFrame % ARRAY_SIZE(animationFall)];

    }
    
    if (animation == LUMBERJACK_DUCK)
    {        
        const int animationDuck[] = {16};
        hero->animationSpeed = 150000;
        return animationDuck[hero->currentFrame % ARRAY_SIZE(animationDuck)];
    }

    if (animation == LUMBERJACK_RUN)
    {
        const int animationRun[] = {7, 8, 9, 10, 11, 12};
        hero->animationSpeed = 90000;        
        return animationRun[hero->currentFrame % ARRAY_SIZE(animationRun)];
    }

    if (animation == LUMBERJACK_IDLE)
    {
        const int animationIdle[] = {0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 3, 1};
        hero->animationSpeed = 150000;
        return animationIdle[hero->currentFrame % ARRAY_SIZE(animationIdle)];
    }

    if (animation == LUMBERJACK_DEAD)
    {
        const int animationDead[] = {14};
        hero->animationSpeed = 150000;
        return animationDead[hero->currentFrame % ARRAY_SIZE(animationDead)];
    }

    if (animation == LUMBERJACK_VICTORY)
    {
        const int animationVictory[] = {15};
        hero->animationSpeed = 150000;
        return animationVictory[hero->currentFrame % ARRAY_SIZE(animationVictory)];

    }
    if (animation == LUMBERJACK_CLIMB)
    {
        const int animationClimb[] = {17 ,18,19,20};
        hero->animationSpeed = 150000;
        return animationClimb[hero->currentFrame % ARRAY_SIZE(animationClimb)];

    }


    return 0;
}