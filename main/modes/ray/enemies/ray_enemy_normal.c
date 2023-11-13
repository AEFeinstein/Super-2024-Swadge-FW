#include "ray_enemy.h"
#include "ray_enemy_normal.h"

/**
 * @brief Run timers for a normal enemy, which include AI, and movement
 *
 * @param ray The entire game state
 * @param enemy The enemy to run timers for
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayEnemyNormalMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Pick a new direction every 2s
    enemy->behaviorTimer -= elapsedUs;
    if (enemy->behaviorTimer <= 0)
    {
        enemy->behaviorTimer += 2000000;

        // Find the vector from the enemy to the player and normalize it
        q24_8 xDiff = SUB_FX(ray->p.posX, enemy->c.posX);
        q24_8 yDiff = SUB_FX(ray->p.posY, enemy->c.posY);

        // Pick the movement direction on the grid
        if (esp_random() % 2 == 0)
        {
            if (xDiff > 0)
            {
                enemy->behavior = MOVE_POS_X;
            }
            else
            {
                enemy->behavior = MOVE_NEG_X;
            }
        }
        else
        {
            if (yDiff > 0)
            {
                enemy->behavior = MOVE_POS_Y;
            }
            else
            {
                enemy->behavior = MOVE_NEG_Y;
            }
        }
    }

    // Reverse behavior if too close to the player
    q24_8 xDist        = SUB_FX(ray->p.posX, enemy->c.posX);
    q24_8 yDist        = SUB_FX(ray->p.posY, enemy->c.posY);
    q24_8 distToPlayer = ADD_FX(MUL_FX(xDist, xDist), MUL_FX(yDist, yDist));
    if (distToPlayer < TO_FX(4))
    {
        switch (enemy->behavior)
        {
            case MOVE_POS_X:
            {
                enemy->behavior = MOVE_NEG_X;
                break;
            }
            case MOVE_NEG_X:
            {
                enemy->behavior = MOVE_POS_X;
                break;
            }
            case MOVE_POS_Y:
            {
                enemy->behavior = MOVE_NEG_Y;
                break;
            }
            case MOVE_NEG_Y:
            {
                enemy->behavior = MOVE_POS_Y;
                break;
            }
            default:
            {
                // Do nothing
                break;
            }
        }
    }

// Player is 40000 * 6
#define SPEED_DENOM (int32_t)(40000 * 18)

    q24_8 delX = 0;
    q24_8 delY = 0;
    switch (enemy->behavior)
    {
        case MOVE_POS_X:
        {
            delX = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        case MOVE_NEG_X:
        {
            delX = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        case MOVE_POS_Y:
        {
            delY = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        case MOVE_NEG_Y:
        {
            delY = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        default:
        {
            // Do nothing
            break;
        }
    }

    q24_8 marginX = (delX > 0 ? 1 : -1) * TO_FX_FRAC(1, 2);
    q24_8 marginY = (delY > 0 ? 1 : -1) * TO_FX_FRAC(1, 2);

    // Move if in bounds
    if (isPassableCell(
            &ray->map.tiles[FROM_FX(enemy->c.posX + delX + marginX)][FROM_FX(enemy->c.posY + delY + marginY)]))
    {
        enemy->c.posX += delX;
        enemy->c.posY += delY;
    }
}

/**
 * @brief Get the time until the next shot is taken
 *
 * @param enemy The enemy taking the shot
 * @param type the timer of timer to get
 * @return The time, in uS, until the next shot
 */
int32_t rayEnemyNormalGetTimer(rayEnemy_t* enemy, rayEnemyTimerType_t type)
{
    return 2000000 + (esp_random() % 2000000);
}

/**
 * @brief Get the bullet this enemy fires
 *
 * @param enemy The shooting enemy
 * @return The bullet type
 */
rayMapCellType_t rayEnemyNormalGetBullet(rayEnemy_t* enemy)
{
    return OBJ_BULLET_E_NORMAL;
}
