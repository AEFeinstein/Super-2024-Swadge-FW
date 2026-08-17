#include "ray_enemy_bush.h"
#include "ray_object.h"
#include "ray_tex_manager.h"

bool rayEnemyBushMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
void rayEnemyBushCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY);
void rayEnemyBushGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
rayMapCellType_t rayEnemyBushDropAfterDeath(ray_t* ray, rayEnemy_t* enemy);

void rayInitEnemyBush(ray_t* ray, rayEnemy_t* e)
{
    // Bush has one texture
    e->c.sprite = loadTexture(ray, OBJ_ENEMY_BUSH_WSG, OBJ_ENEMY_BUSH);

    // Bush has 1hp
    e->health = 1;

    // No enemy-specific state
    e->state = NULL;

    // Set function pointers
    e->mainFn           = rayEnemyBushMain;
    e->collisionFn      = rayEnemyBushCheckPlayerCollision;
    e->getShotFn        = rayEnemyBushGetShot;
    e->dropAfterDeathFn = rayEnemyBushDropAfterDeath;
}

bool rayEnemyBushMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    return enemy->health <= 0;
}

void rayEnemyBushCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
{
    rectangle_t enemyBB = rayGetObjBB(&enemy->c);
    if (rectRectIntersection(player, enemyBB, NULL))
    {
        *deltaX = 0;
        *deltaY = 0;
    }
}

void rayEnemyBushGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // Swords and bombs break bushes
    switch (bullet)
    {
        case OBJ_BULLET_BOMB:
        case OBJ_BULLET_SWORD:
        {
            enemy->health = 0;
            return;
        }
        default:
        {
            return;
        }
    }
}

rayMapCellType_t rayEnemyBushDropAfterDeath(ray_t* ray, rayEnemy_t* enemy)
{
    switch ((uint8_t)(esp_random() & 0xFF))
    {
        case 0 ... 46:
        {
            // 18.5% chance of 1 mpoint
            return OBJ_ITEM_MPOINT_1;
        }
        case 47 ... 55:
        {
            // 3.7% chance of 5 mpoint
            return OBJ_ITEM_MPOINT_5;
        }
        case 56 ... 60:
        {
            // 1.9% chance of 10 mpoint
            return OBJ_ITEM_MPOINT_10;
        }
        case 61 ... 62:
        {
            // 0.9% chance of 20 mpoint
            return OBJ_ITEM_MPOINT_20;
        }
        case 63 ... 127:
        {
            // 25% chance of heart
            return OBJ_ITEM_HEART;
        }
        default:
        {
            // 50% chance of nothing
            return EMPTY;
        }
    }
}
