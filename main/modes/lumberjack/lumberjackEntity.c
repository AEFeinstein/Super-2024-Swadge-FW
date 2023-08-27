#include "lumberjack_types.h"
#include "lumberjackEntity.h"



void lumberjackSetupEnemy(lumberjackEntity_t* enemy, int character)
{
    enemy->direction = 1;
    enemy->width = 15;
    enemy->height = 15;
    enemy->tileHeight = 1;
    enemy->state = LUMBERJACK_RUN;
    enemy->maxVX = 0;
    enemy->active = true;

    if (character == 0)
    {

        enemy->maxVX = 5;
        loadWsg("enemy_a1.wsg", &enemy->frames[0], true);
        loadWsg("enemy_a2.wsg", &enemy->frames[1], true);
        loadWsg("enemy_a3.wsg", &enemy->frames[2], true);
        loadWsg("enemy_a4.wsg", &enemy->frames[3], true);
        loadWsg("enemy_a5.wsg", &enemy->frames[4], true);
        loadWsg("enemy_a6.wsg", &enemy->frames[5], true);
        loadWsg("enemy_a7.wsg", &enemy->frames[6], true);
    }


    enemy->type = character;

}

void lumberjackDoEnemyControls(lumberjackEntity_t* enemy)
{
    //pick between types I guess

}

void lumberjackUnloadEnemy(lumberjackEntity_t* enemy)
{
    int frames = 8;

    if (enemy->type == 0) frames = 8;

    for (int i = 0; i< frames; i++)
    {
        //if (&enemy->frames[i] == NULL) continue;
        
        freeWsg(&enemy->frames[i]);
    }

}

uint8_t lumberjackGetEnemyAnimation(lumberjackEntity_t* enemy)
{
    int animation = enemy->state;
    enemy->animationSpeed= 150000;

    if (animation == LUMBERJACK_DEAD)
    {
        return 5;
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
