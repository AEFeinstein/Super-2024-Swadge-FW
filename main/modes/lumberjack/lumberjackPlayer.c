
#include "lumberjack_types.h"
#include "lumberjackEntity.h"
#include "lumberjackPlayer.h"

#include <esp_log.h>

#define LUMBERJACK_DEFAULT_ANIMATION_SPEED 150000
#define LUMBERJACK_INVINCIBLE_ANIMATION_SPEED 25000
#define LUMBERJACK_SPAWN_Y                 270
#define LUMBERJACK_HERO_WIDTH              24
#define LUMBERJACK_HERO_HEIGHT             31
#define LUMBERJACK_HERO_DUCK_HEIGHT        31

void lumberjackSetupPlayer(lumberjackEntity_t* hero, int character)
{
    hero->height       = LUMBERJACK_HERO_HEIGHT;
    hero->width        = LUMBERJACK_HERO_WIDTH;
    hero->tileHeight   = 2;
    hero->maxVX        = 15;
    hero->active       = true;
    hero->showAlert    = false;
    hero->upgrading    = false;
    hero->spriteOffset = 0;
    hero->maxLevel     = character;
    hero->type         = character;
    hero->state        = LUMBERJACK_UNSPAWNED;
    hero->lives        = 3;

}

void lumberjackSpawnPlayer(lumberjackEntity_t* hero, int x, int y, int facing)
{
    hero->x                = x;
    hero->y                = LUMBERJACK_SPAWN_Y;
    hero->vx               = 0;
    hero->maxVX            = 15;
    hero->vy               = 0;
    hero->flipped          = (facing == 0);
    hero->state            = LUMBERJACK_INVINCIBLE;
    hero->timerFrameUpdate = 0;
    hero->active           = true;
    hero->onGround         = true;
    hero->ready            = false;
    hero->respawn          = 0;
}

void lumberjackRespawn(lumberjackEntity_t* hero)
{
    hero->x                = 130;
    hero->maxVX            = 15;
    hero->y                = LUMBERJACK_SPAWN_Y;
    hero->active           = true;
    hero->ready            = false;
    hero->vx               = 0;
    hero->vy               = 0;
    hero->flipped          = 0;
    hero->state            = LUMBERJACK_INVINCIBLE;
    hero->timerFrameUpdate = 0;
    hero->onGround         = true;
    hero->maxLevel         = 0;
    hero->submergedTimer   = 5000;

}

int lumberjackGetPlayerAnimation(lumberjackEntity_t* hero)
{
    // int animationNone[] = {0};

    int animation        = hero->state;
    hero->animationSpeed = LUMBERJACK_DEFAULT_ANIMATION_SPEED;
    hero->height         = LUMBERJACK_HERO_HEIGHT;

    if (hero->onGround == false && hero->jumping == false && hero->active)
    {
        const int animationFall[] = {13};
        return animationFall[hero->currentFrame % ARRAY_SIZE(animationFall)];
    }

    if (animation == LUMBERJACK_DUCK)
    {
        const int animationDuck[] = {16};
        hero->height              = LUMBERJACK_HERO_DUCK_HEIGHT;
        hero->animationSpeed      = LUMBERJACK_DEFAULT_ANIMATION_SPEED;
        return animationDuck[hero->currentFrame % ARRAY_SIZE(animationDuck)];
    }

    if (animation == LUMBERJACK_RUN)
    {
        const int animationRun[] = {7, 8, 9, 10, 11, 12};
        hero->animationSpeed     = 90000;
        return animationRun[hero->currentFrame % ARRAY_SIZE(animationRun)];
    }

    if (animation == LUMBERJACK_IDLE)
    {
        const int animationIdle[] = {0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 2, 1, 0, 1, 3, 1};
        hero->animationSpeed      = LUMBERJACK_DEFAULT_ANIMATION_SPEED;
        return animationIdle[hero->currentFrame % ARRAY_SIZE(animationIdle)];
    }

    if (animation == LUMBERJACK_DEAD)
    {
        const int animationDead[] = {14};
        hero->animationSpeed      = LUMBERJACK_DEFAULT_ANIMATION_SPEED;
        return animationDead[hero->currentFrame % ARRAY_SIZE(animationDead)];
    }

    if (animation == LUMBERJACK_VICTORY)
    {
        const int animationVictory[] = {15};
        hero->animationSpeed         = LUMBERJACK_DEFAULT_ANIMATION_SPEED;
        return animationVictory[hero->currentFrame % ARRAY_SIZE(animationVictory)];
    }
    if (animation == LUMBERJACK_INVINCIBLE)
    {
        const int animationInvincible[] = {0, 17};
        hero->animationSpeed       = LUMBERJACK_INVINCIBLE_ANIMATION_SPEED;
        return animationInvincible[hero->currentFrame % ARRAY_SIZE(animationInvincible)];
    }

    return 0;
}