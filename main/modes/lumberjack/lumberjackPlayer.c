#include "lumberjackPlayer.h"
#include "lumberjack_types.h"

void lumberjackSetupPlayer(lumberjackHero_t* hero, int character)
{
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
    else
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

    }
    
}

void lumberjackSpawnPlayer(lumberjackHero_t* hero, int x, int y, int facing)
{
    hero->x = x;
    hero->y = y;
    hero->flipped = (facing == 0);
    hero->state = LUMBERJACK_IDLE;
}

int lumberjackGetPlayerAnimation(lumberjackHero_t* hero)
{
    int animation =  hero->state;
    if (animation == LUMBERJACK_DUCK)
    {        
        hero->animationSpeed = 150000;
        return lumberjackAnimationDuck[hero->currentFrame % ((int)( sizeof(lumberjackAnimationDuck) / sizeof(lumberjackAnimationDuck[0])) )];
    }

    if (animation == LUMBERJACK_RUN)
    {
        hero->animationSpeed = 90000;        
        return lumberjackAnimationRun[hero->currentFrame % ((int)( sizeof(lumberjackAnimationRun) / sizeof(lumberjackAnimationRun[0])) )];
    }

    if (animation == LUMBERJACK_IDLE)
    {
        hero->animationSpeed = 150000;
        return lumberjackAnimationIdle[hero->currentFrame % ((int)( sizeof(lumberjackAnimationIdle) / sizeof(lumberjackAnimationIdle[0])) )];
    }

    if (animation == LUMBERJACK_DEAD)
    {
        hero->animationSpeed = 150000;
        return lumberjackAnimationDead[hero->currentFrame % ((int)( sizeof(lumberjackAnimationDead) / sizeof(lumberjackAnimationDead[0])) )];
    }

    if (animation == LUMBERJACK_VICTORY)
    {
        hero->animationSpeed = 150000;
        return lumberjackAnimationVictory[hero->currentFrame % ((int)( sizeof(lumberjackAnimationVictory) / sizeof(lumberjackAnimationVictory[0])) )];

    }
    if (animation == LUMBERJACK_CLIMB)
    {
        hero->animationSpeed = 150000;
        return lumberjackAnimationClimb[hero->currentFrame % ((int)( sizeof(lumberjackAnimationClimb) / sizeof(lumberjackAnimationClimb[0])) )];

    }


    return 0;
}