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

    q24_8 delX    = 0;
    q24_8 delY    = 0;
    q24_8 marginX = 0;
    q24_8 marginY = 0;
    switch (enemy->behavior)
    {
        case MOVE_NE:
        {
            delX    = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY    = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            marginX = TO_FX_FRAC(1, 2);
            marginY = -TO_FX_FRAC(1, 2);
            break;
        }
        case MOVE_SE:
        {
            delX    = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY    = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            marginX = TO_FX_FRAC(1, 2);
            marginY = TO_FX_FRAC(1, 2);
            break;
        }
        case MOVE_SW:
        {
            delX    = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY    = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            marginX = -TO_FX_FRAC(1, 2);
            marginY = TO_FX_FRAC(1, 2);
            break;
        }
        case MOVE_NW:
        {
            delX    = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY    = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            marginX = -TO_FX_FRAC(1, 2);
            marginY = -TO_FX_FRAC(1, 2);
            break;
        }
        default:
        {
            break;
        }
    }

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
 * @brief This is called when a flaming enemy is shot. It adds damage based on bullet type, checks scripts, and handles
 * freeing defeated enemies
 *
 * @param ray The entire game state
 * @param enemy The enemy which was shot
 * @param bullet The type of bullet it was shot by
 * @return true if the enemy was killed, false if it's still alive
 */
bool rayEnemyFlamingGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // Health starts at 100
    bool hurt = false;
    switch (bullet)
    {
        case OBJ_BULLET_ICE:
        {
            // Two shots to kill
            enemy->health -= 50;
            hurt = true;
            break;
        }
        case OBJ_BULLET_NORMAL:
        case OBJ_BULLET_CHARGE:
        case OBJ_BULLET_XRAY:
        case OBJ_BULLET_MISSILE:
        {
            // Five shots to kill
            enemy->health -= 20;
            hurt = true;
            break;
        }
        default:
        {
            // Not a bullet
            break;
        }
    }

    if (hurt)
    {
        rayEnemyTransitionState(enemy, E_HURT);
    }
    return enemy->health <= 0;
}

/**
 * @brief Get the time until the next shot is taken
 *
 * @param enemy The enemy taking the shot
 * @return The time, in uS, until the next shot
 */
int32_t rayEnemyFlamingGetShotTimer(rayEnemy_t* enemy)
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
    return OBJ_BULLET_ICE;
}
