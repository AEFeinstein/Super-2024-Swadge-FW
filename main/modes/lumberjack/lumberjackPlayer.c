
#include "lumberjack_types.h"
#include "lumberjackEntity.h"
#include "lumberjackPlayer.h"

void lumberjackSetupPlayer(lumberjackEntity_t* hero, int character)
{
    hero->height = 31;
    hero->width = 31;
    hero->tileHeight = 2;
    hero->maxVX = 15;
    hero->active = true;
    if (character == 0)
    {
        loadWsg("lumbers_red_1.wsg", &hero->frames[0], true);
        loadWsg("lumbers_red_2.wsg", &hero->frames[1], true);
        loadWsg("lumbers_red_3.wsg", &hero->frames[2], true);
        loadWsg("lumbers_red_4.wsg", &hero->frames[3], true);
        loadWsg("lumbers_red_5.wsg", &hero->frames[4], true);
        loadWsg("lumbers_red_6.wsg", &hero->frames[5], true);
        loadWsg("lumbers_red_7.wsg", &hero->frames[6], true);
        loadWsg("lumbers_red_8.wsg", &hero->frames[7], true);
        loadWsg("lumbers_red_9.wsg", &hero->frames[8], true);
        loadWsg("lumbers_red_10.wsg", &hero->frames[9], true);
        loadWsg("lumbers_red_11.wsg", &hero->frames[10], true);
        loadWsg("lumbers_red_12.wsg", &hero->frames[11], true);
        loadWsg("lumbers_red_13.wsg", &hero->frames[12], true);
        loadWsg("lumbers_red_14.wsg", &hero->frames[13], true);
        loadWsg("lumbers_red_15.wsg", &hero->frames[14], true);
        loadWsg("lumbers_red_16.wsg", &hero->frames[15], true);
        loadWsg("lumbers_red_17.wsg", &hero->frames[16], true);
        loadWsg("lumbers_red_18.wsg", &hero->frames[17], true);
        loadWsg("lumbers_red_19.wsg", &hero->frames[18], true);
        loadWsg("lumbers_red_20.wsg", &hero->frames[19], true);
        loadWsg("lumbers_red_21.wsg", &hero->frames[20], true);
    }
    else if (character == 1)
    {
        loadWsg("lumbers_green_1.wsg", &hero->frames[0], true);
        loadWsg("lumbers_green_2.wsg", &hero->frames[1], true);
        loadWsg("lumbers_green_3.wsg", &hero->frames[2], true);
        loadWsg("lumbers_green_4.wsg", &hero->frames[3], true);
        loadWsg("lumbers_green_5.wsg", &hero->frames[4], true);
        loadWsg("lumbers_green_6.wsg", &hero->frames[5], true);
        loadWsg("lumbers_green_7.wsg", &hero->frames[6], true);
        loadWsg("lumbers_green_8.wsg", &hero->frames[7], true);
        loadWsg("lumbers_green_9.wsg", &hero->frames[8], true);
        loadWsg("lumbers_green_10.wsg", &hero->frames[9], true);
        loadWsg("lumbers_green_11.wsg", &hero->frames[10], true);
        loadWsg("lumbers_green_12.wsg", &hero->frames[11], true);
        loadWsg("lumbers_green_13.wsg", &hero->frames[12], true);
        loadWsg("lumbers_green_14.wsg", &hero->frames[13], true);
        loadWsg("lumbers_green_15.wsg", &hero->frames[14], true);
        loadWsg("lumbers_green_16.wsg", &hero->frames[15], true);
        loadWsg("lumbers_green_17.wsg", &hero->frames[16], true);
        loadWsg("lumbers_green_18.wsg", &hero->frames[17], true);
        loadWsg("lumbers_green_19.wsg", &hero->frames[18], true);
        loadWsg("lumbers_green_20.wsg", &hero->frames[19], true);
        loadWsg("lumbers_green_21.wsg", &hero->frames[20], true);

    } else 
    {

        loadWsg("secret_swadgeland_1.wsg", &hero->frames[0], true);
        loadWsg("secret_swadgeland_2.wsg", &hero->frames[1], true);
        loadWsg("secret_swadgeland_3.wsg", &hero->frames[2], true);
        loadWsg("secret_swadgeland_4.wsg", &hero->frames[3], true);
        loadWsg("secret_swadgeland_5.wsg", &hero->frames[4], true);
        loadWsg("secret_swadgeland_6.wsg", &hero->frames[5], true);
        loadWsg("secret_swadgeland_7.wsg", &hero->frames[6], true);
        loadWsg("secret_swadgeland_8.wsg", &hero->frames[7], true);
        loadWsg("secret_swadgeland_9.wsg", &hero->frames[8], true);
        loadWsg("secret_swadgeland_10.wsg", &hero->frames[9], true);
        loadWsg("secret_swadgeland_11.wsg", &hero->frames[10], true);
        loadWsg("secret_swadgeland_12.wsg", &hero->frames[11], true);
        loadWsg("secret_swadgeland_13.wsg", &hero->frames[12], true);
        loadWsg("secret_swadgeland_14.wsg", &hero->frames[13], true);
        loadWsg("secret_swadgeland_15.wsg", &hero->frames[14], true);
        loadWsg("secret_swadgeland_16.wsg", &hero->frames[15], true);
        loadWsg("secret_swadgeland_17.wsg", &hero->frames[16], true);
        loadWsg("secret_swadgeland_18.wsg", &hero->frames[17], true);
        loadWsg("secret_swadgeland_19.wsg", &hero->frames[18], true);
        loadWsg("secret_swadgeland_20.wsg", &hero->frames[19], true);
        loadWsg("secret_swadgeland_21.wsg", &hero->frames[20], true);
    }
    
}

void lumberjackSpawnPlayer(lumberjackEntity_t* hero, int x, int y, int facing)
{
    hero->x = x;
    hero->y = 270;
    hero->vx = 0;
    hero->vy = 0;
    hero->flipped = (facing == 0);
    hero->state = LUMBERJACK_IDLE;
    hero->timerFrameUpdate = 0;
    hero->active = true;
    hero->onGround = true;
}

void lumberjackRespawn(lumberjackEntity_t* hero)
{
    hero->x = 130;
    hero->y = 270;
    hero->active = true;
    hero->vx = 0;
    hero->vy = 0;
    hero->flipped = 0;
    hero->state = LUMBERJACK_IDLE;
    hero->timerFrameUpdate = 0;
    hero->onGround = true;
}

int lumberjackGetPlayerAnimation(lumberjackEntity_t* hero)
{
    int animationNone[] = {0};

    int animation =  hero->state;
    hero->animationSpeed = 150000;

    if (hero->onGround == false && hero->jumping == false && hero->active)
    {
        int animationFall[] = {13};
        return animationFall[hero->currentFrame % ((int)( sizeof(animationFall) / sizeof(animationFall[0])) )];

    }
    
    if (animation == LUMBERJACK_DUCK)
    {        
        int animationDuck[] = {16};
        hero->animationSpeed = 150000;
        return animationDuck[hero->currentFrame % ((int)( sizeof(animationDuck) / sizeof(animationDuck[0])) )];
    }

    if (animation == LUMBERJACK_RUN)
    {
        int animationRun[] = {7, 8, 9, 10, 11, 12};
        hero->animationSpeed = 90000;        
        return animationRun[hero->currentFrame % ((int)( sizeof(animationRun) / sizeof(animationRun[0])) )];
    }

    if (animation == LUMBERJACK_IDLE)
    {
        int animationIdle[] = {0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 3, 1};
        hero->animationSpeed = 150000;
        return animationIdle[hero->currentFrame % ((int)( sizeof(animationIdle) / sizeof(animationIdle[0])) )];
    }

    if (animation == LUMBERJACK_DEAD)
    {
        int animationDead[] = {14};
        hero->animationSpeed = 150000;
        return animationDead[hero->currentFrame % ((int)( sizeof(animationDead) / sizeof(animationDead[0])) )];
    }

    if (animation == LUMBERJACK_VICTORY)
    {
        int animationVictory[] = {15};
        hero->animationSpeed = 150000;
        return animationVictory[hero->currentFrame % ((int)( sizeof(animationVictory) / sizeof(animationVictory[0])) )];

    }
    if (animation == LUMBERJACK_CLIMB)
    {
        int animationClimb[] = {17 ,18,19,20};
        hero->animationSpeed = 150000;
        return animationClimb[hero->currentFrame % ((int)( sizeof(animationClimb) / sizeof(animationClimb[0])) )];

    }


    return 0;
}