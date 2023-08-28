#include "lumberjack_types.h"
#include "lumberjackEntity.h"
#include "lumberjack.h"


void lumberjackSetupEnemy(lumberjackEntity_t* enemy, int character)
{
    enemy->direction = 1;
    enemy->state = LUMBERJACK_RUN;
    enemy->maxVX = 0;
    enemy->active = false;
    enemy->ready = true; 
    enemy->showAlert = false;
    enemy->spriteOffset = 0;

    lumberjackUpdateEnemy(enemy, character);
    
}

void lumberjackResetEnemy(lumberjackEntity_t* enemy)
{
    enemy->direction = 1; // This needs to be decided ahead of time
    enemy->tileHeight = 1;
    if (enemy->state != LUMBERJACK_DEAD) enemy->state = LUMBERJACK_RUN;
    enemy->active = false;
    enemy->ready = true;
    enemy->y = 0;
    enemy->x = 0;
    enemy->upgrading = false;

}

void lumberjackRespawnEnemy(lumberjackEntity_t* enemy, int side)
{
    enemy->tileHeight = 1;
    enemy->state = LUMBERJACK_RUN;
    enemy->active = true;
    enemy->ready = false;
    enemy->y = 0;
    
    //I need to figure out why when moving right he appears to move faster
    if (side == 1)
    {
        enemy->direction = 1; // This needs to be decided ahead of time
        enemy->x = 0;
        enemy->vx = 0;//enemy->maxVX;
        enemy->flipped = false;
    }
    else
{
        enemy->direction = -1; // This needs to be decided ahead of time
        enemy->x = 200;
        enemy->vx = 0;// -enemy->maxVX;
        enemy->flipped = true;        
    }
}

void lumberjackUpdateEnemy(lumberjackEntity_t* enemy, int newIndex)
{
    
   
    enemy->type = newIndex;

    if (enemy->type > enemy->maxLevel)
    {
        enemy->type = enemy->maxLevel;
    }

    enemy->upgrading = false;

    if (newIndex == 0)
    {
        enemy->width = 15;
        enemy->height = 15;
        enemy->tileHeight = 1;
        enemy->maxVX = 4;
        enemy->spriteOffset = 0;
        enemy->maxLevel = 2;

    }
    else if (newIndex == 1)
    {
        enemy->width = 15;
        enemy->height = 15;
        enemy->tileHeight = 1;
        enemy->maxVX = 8;
        enemy->spriteOffset = 7;
        enemy->maxLevel = 2;

    }
    else if (newIndex == 2)
    {
        enemy->width = 15;
        enemy->height = 15;
        enemy->tileHeight = 1;
        enemy->maxVX = 10;
        enemy->spriteOffset = 14;
        enemy->maxLevel = 2;

    }


    enemy->type = newIndex;

}

void lumberjackDoEnemyControls(lumberjackEntity_t* enemy)
{
    //pick between types I guess
    //if enemy->type 1, 2, or 3... continue

    
}


uint8_t lumberjackGetEnemyAnimation(lumberjackEntity_t* enemy)
{
    int animation = enemy->state;
    enemy->animationSpeed= 150000;

    if (animation == LUMBERJACK_DEAD)
    {
        return 5;
    }

    if (enemy->upgrading)
    {
        enemy->animationSpeed /=2;
    }

    if (animation == LUMBERJACK_RUN)
    {
        int anim[] = {0, 1, 2, 3};
        return anim[enemy->currentFrame % 4];
    }

    if (animation == LUMBERJACK_BUMPED)
    {
        int anim[] = {4};
        return anim[enemy->currentFrame % 2];
    }


    if (animation == LUMBERJACK_BUMPED_IDLE)
    {
        int anim[] = {5, 6};
        return anim[enemy->currentFrame % 2];
    }

    return 0;
}

bool checkCollision(lumberjackEntity_t* AA, lumberjackEntity_t* BB)
{
    return (AA->x < BB->x + BB->width &&
                AA->x + AA->width > BB->x &&
                AA->y < BB->y + BB->height &&
                AA->y + AA->height > BB->y);
                
}
