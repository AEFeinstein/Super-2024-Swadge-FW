#include "ray_enemy.h"
#include "ray_enemy_flaming.h"

/**
 * @brief Run timers for a flaming enemy, which include AI, and movement
 *
 * @param ray The entire game state
 * @param enemy The enemy to run timers for
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayEnemyFlamingMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // If the enemy is doing nothing
    if (DOING_NOTHING == enemy->behavior)
    {
        // Pick a random starting direction
        enemy->behavior = MOVE_NE + (esp_random() % 4);
    }

// Player is 40000 * 6
#define SPEED_DENOM (int32_t)(40000 * 18)

    q24_8 delX = 0;
    q24_8 delY = 0;
    switch (enemy->behavior)
    {
        case MOVE_NE:
        {
            delX = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        case MOVE_SE:
        {
            delX = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        case MOVE_SW:
        {
            delX = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        case MOVE_NW:
        {
            delX = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        default:
        {
            break;
        }
    }

    q24_8 marginX = (delX > 0 ? 1 : -1) * TO_FX_FRAC(1, 2);
    q24_8 marginY = (delY > 0 ? 1 : -1) * TO_FX_FRAC(1, 2);

    // If the cell can be moved into
    if (isPassableCell(
            &ray->map.tiles[FROM_FX(enemy->c.posX + delX + marginX)][FROM_FX(enemy->c.posY + delY + marginY)]))
    {
        // Move into it
        enemy->c.posX += delX;
        enemy->c.posY += delY;
    }
    else if (!isPassableCell(&ray->map.tiles[FROM_FX(enemy->c.posX + delX + marginX)][FROM_FX(enemy->c.posY)]))
    {
        // Collision on X axis, invert that
        switch (enemy->behavior)
        {
            case MOVE_NE:
            {
                enemy->behavior = MOVE_NW;
                break;
            }
            case MOVE_SE:
            {
                enemy->behavior = MOVE_SW;
                break;
            }
            case MOVE_SW:
            {
                enemy->behavior = MOVE_SE;
                break;
            }
            case MOVE_NW:
            {
                enemy->behavior = MOVE_NE;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    else
    {
        // Collision on Y axis, invert that
        switch (enemy->behavior)
        {
            case MOVE_NE:
            {
                enemy->behavior = MOVE_SE;
                break;
            }
            case MOVE_SE:
            {
                enemy->behavior = MOVE_NE;
                break;
            }
            case MOVE_SW:
            {
                enemy->behavior = MOVE_NW;
                break;
            }
            case MOVE_NW:
            {
                enemy->behavior = MOVE_SW;
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

/**
 * @brief Get the time until the next shot is taken
 *
 * @param enemy The enemy taking the shot
 * @param type the timer of timer to get
 * @return The time, in uS, until the next shot
 */
int32_t rayEnemyFlamingGetTimer(rayEnemy_t* enemy, rayEnemyTimerType_t type)
{
    return 2000000 + (esp_random() % 2000000);
}

/**
 * @brief Get the bullet this enemy fires
 *
 * @param enemy The shooting enemy
 * @return The bullet type
 */
rayMapCellType_t rayEnemyFlamingGetBullet(rayEnemy_t* enemy)
{
    return OBJ_BULLET_E_FLAMING;
}
