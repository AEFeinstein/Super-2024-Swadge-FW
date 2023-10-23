#include "ray_enemy.h"
#include "ray_enemy_boss.h"

/**
 * @brief Run timers for a boss enemy, which include AI, and movement
 *
 * @param ray The entire game state
 * @param enemy The enemy to run timers for
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayEnemyBossMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Pick a new weakness every 4s
    enemy->behaviorTimer += elapsedUs;
    if (enemy->behaviorTimer > 4000000)
    {
        enemy->behaviorTimer -= 4000000;

        enemy->bossState = (enemy->bossState + (1 + esp_random() % (NUM_BOSS_STATES - 1))) % NUM_BOSS_STATES;

        if (B_NORMAL == enemy->bossState)
        {
            enemy->sprites = &ray->enemyTex[OBJ_ENEMY_BOSS - OBJ_ENEMY_NORMAL];
        }
        else
        {
            enemy->sprites = &ray->bossTex[enemy->bossState];
        }

        enemy->c.sprite = &enemy->sprites[0][enemy->state][enemy->animFrame];
    }
}

/**
 * @brief This is called when a boss enemy is shot. It adds damage based on bullet type, checks scripts, and handles
 * freeing defeated enemies
 *
 * @param ray The entire game state
 * @param enemy The enemy which was shot
 * @param bullet The type of bullet it was shot by
 * @return true if the enemy was killed, false if it's still alive
 */
bool rayEnemyBossGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    bool hurt = false;
    switch (enemy->bossState)
    {
        case B_NORMAL:
        {
            if (OBJ_BULLET_NORMAL == bullet)
            {
                enemy->health -= 5;
                hurt = true;
            }
            else if (OBJ_BULLET_CHARGE == BULLET)
            {
                enemy->health -= 10;
                hurt = true;
            }
            break;
        }
        case B_MISSILE:
        {
            if (OBJ_BULLET_MISSILE == bullet)
            {
                enemy->health -= 5;
                hurt = true;
            }
            break;
        }
        case B_ICE:
        {
            if (OBJ_BULLET_ICE == bullet)
            {
                enemy->health -= 5;
                hurt = true;
            }
            break;
        }
        case B_XRAY:
        {
            if (OBJ_BULLET_XRAY == bullet)
            {
                enemy->health -= 5;
                hurt = true;
            }
            break;
        }
    }

    if (hurt)
    {
        rayEnemyTransitionState(enemy, E_HURT);
    }

    return (enemy->health <= 0);
}
