#include <esp_log.h>
#include "lumberjack_types.h"
#include "lumberjackEntity.h"
#include "lumberjack.h"

void lumberjackSetupEnemy(lumberjackEntity_t* enemy, int character)
{
    enemy->direction      = 1;
    enemy->state          = LUMBERJACK_RUN;
    enemy->maxVX          = 0;
    enemy->active         = false;
    enemy->ready          = true;
    enemy->showAlert      = false;
    enemy->spriteOffset   = 0;
    enemy->cW             = 15;
    enemy->cH             = 15;
    enemy->submergedTimer = 0;
    enemy->queueable      = true;
    lumberjackUpdateEnemy(enemy, character);
}

void lumberjackResetEnemy(lumberjackEntity_t* enemy)
{
    enemy->direction  = 1; // This needs to be decided ahead of time
    enemy->tileHeight = 1;
    if (enemy->state != LUMBERJACK_DEAD)
        enemy->state = LUMBERJACK_RUN;
    enemy->active    = false;
    enemy->ready     = true;
    enemy->y         = 0;
    enemy->x         = 0;
    enemy->upgrading = false;

    lumberjackUpdateEnemy(enemy, enemy->type);
}

void lumberjackRespawnEnemy(lumberjackEntity_t* enemy, int side)
{
    enemy->tileHeight = 1;
    enemy->state      = LUMBERJACK_RUN;
    enemy->active     = true;
    enemy->ready      = false;
    enemy->y          = 0;
    enemy->lives      = 1;

    // I need to figure out why when moving right he appears to move faster
    if (side == 1)
    {
        enemy->direction = 1; // This needs to be decided ahead of time
        enemy->x         = 0;
        enemy->vx        = 0; // enemy->maxVX;
        enemy->flipped   = false;
    }
    else
    {
        enemy->direction = -1; // This needs to be decided ahead of time
        enemy->x         = 279;
        enemy->vx        = 0; // -enemy->maxVX;
        enemy->flipped   = true;
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
        enemy->width        = 15;
        enemy->height       = 15;
        enemy->tileHeight   = 1;
        enemy->maxVX        = 4;
        enemy->spriteOffset = 0;
        enemy->maxLevel     = 2;
        enemy->scoreValue   = 100;
        enemy->type         = newIndex;

        enemy->cW = 15;
        enemy->cH = 15;
    }
    else if (newIndex == 1)
    {
        enemy->width        = 15;
        enemy->height       = 15;
        enemy->tileHeight   = 1;
        enemy->maxVX        = 6;
        enemy->spriteOffset = 7;
        enemy->maxLevel     = 2;
        enemy->cW           = 15;
        enemy->scoreValue   = 250;
        enemy->cH           = 15;
        enemy->type         = newIndex;
    }
    else if (newIndex == 2)
    {
        enemy->width        = 15;
        enemy->height       = 15;
        enemy->tileHeight   = 1;
        enemy->maxVX        = 8;
        enemy->spriteOffset = 14;
        enemy->maxLevel     = 2;
        enemy->cW           = 15;
        enemy->scoreValue   = 500;
        enemy->cH           = 15;
        enemy->type         = newIndex;
    }
    else if (newIndex == 4)
    {
        enemy->width        = 15;
        enemy->height       = 15;
        enemy->tileHeight   = 1;
        enemy->maxVX        = 10;
        enemy->spriteOffset = 23;
        enemy->maxLevel     = 6;
        enemy->cW           = 15;
        enemy->scoreValue   = 500;
        enemy->cH           = 15;
        enemy->type         = newIndex;
    }
    else if (newIndex == 5)
    {
        enemy->width        = 15;
        enemy->height       = 15;
        enemy->tileHeight   = 1;
        enemy->maxVX        = 10;
        enemy->spriteOffset = 30;
        enemy->maxLevel     = 6;
        enemy->cW           = 15;
        enemy->scoreValue   = 500;
        enemy->cH           = 15;
        enemy->type         = newIndex;
    }
}

void lumberjackUpdateEnemyCollision(lumberjackEntity_t* enemy)
{
    enemy->cX = enemy->x;
    enemy->cY = enemy->y;
}

void lumberjackUpdatePlayerCollision(lumberjackEntity_t* player)
{
    player->cX = player->x + 2;
    player->cW = player->width - 4;

    if (player->state == LUMBERJACK_DUCK)
    {
        player->cY = player->y + 15;
        player->cH = player->height - 14;
    }
    else
    {
        player->cY = player->y + 4;
        player->cH = player->height - 8;
    }
}

uint8_t lumberjackGetEnemyAnimation(lumberjackEntity_t* enemy)
{
    int animation         = enemy->state;
    enemy->animationSpeed = 150000;

    if (animation == LUMBERJACK_DEAD)
    {
        return 5;
    }

    if (enemy->upgrading)
    {
        enemy->animationSpeed /= 2;
    }

    if (animation == LUMBERJACK_RUN)
    {
        const int anim[] = {0, 1, 2, 3};
        return anim[enemy->currentFrame % 4];
    }

    if (animation == LUMBERJACK_BUMPED)
    {
        const int anim[] = {4};
        return anim[0];
    }

    if (animation == LUMBERJACK_BUMPED_IDLE)
    {
        const int anim[] = {5, 6};
        return anim[enemy->currentFrame % 2];
    }

    return 0;
}

bool checkCollision(lumberjackEntity_t* AA, lumberjackEntity_t* BB)
{
    return (AA->x < BB->x + BB->width && AA->x + AA->width > BB->x && AA->y < BB->y + BB->height
            && AA->y + AA->height > BB->y);
}
