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
    // Pick an initial direction to move in
    q24_8 xDiff = SUB_FX(ray->p.posX, enemy->c.posX);
    q24_8 yDiff = SUB_FX(ray->p.posY, enemy->c.posY);

    // If the enemy is doing nothing
    if (DOING_NOTHING == enemy->behavior)
    {
        // Move orthogonal to the player
        if (ABS(xDiff) > ABS(yDiff))
        {
            enemy->behavior = MOVE_POS_Y;
        }
        else
        {
            enemy->behavior = MOVE_POS_X;
        }
    }

// Player is 40000 * 6
#define SPEED_DENOM (int32_t)(40000 * 18)

    q24_8 delX    = 0;
    q24_8 delY    = 0;
    q24_8 marginX = 0;
    q24_8 marginY = 0;
    switch (enemy->behavior)
    {
        case MOVE_POS_X:
        {
            delX    = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            marginX = TO_FX_FRAC(1, 2);
            break;
        }
        case MOVE_NEG_X:
        {
            delX    = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            marginX = -TO_FX_FRAC(1, 2);
            break;
        }
        case MOVE_POS_Y:
        {
            delY    = (TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            marginY = TO_FX_FRAC(1, 2);
            break;
        }
        case MOVE_NEG_Y:
        {
            delY    = -(TO_FX(1) * (int32_t)(elapsedUs)) / SPEED_DENOM;
            marginY = -TO_FX_FRAC(1, 2);
            break;
        }
        default:
        {
            // Do nothing
            break;
        }
    }

    // Move if in bounds
    if (isPassableCell(
            &ray->map.tiles[FROM_FX(enemy->c.posX + delX + marginX)][FROM_FX(enemy->c.posY + delY + marginY)]))
    {
        enemy->c.posX += delX;
        enemy->c.posY += delY;
    }
    else
    {
        // Bounce off walls
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
}

/**
 * @brief This is called when a normal enemy is shot. It adds damage based on bullet type, checks scripts, and handles
 * freeing defeated enemies
 *
 * @param ray The entire game state
 * @param enemy The enemy which was shot
 * @param bullet The type of bullet it was shot by
 * @return true if the enemy was killed, false if it's still alive
 */
bool rayEnemyNormalGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // Health starts at 100
    bool hurt = false;
    switch (bullet)
    {
        case OBJ_BULLET_NORMAL:
        {
            // Three shots to kill
            enemy->health -= 34;
            hurt = true;
            break;
        }
        case OBJ_BULLET_CHARGE:
        {
            // Two shots to kill
            enemy->health -= 50;
            hurt = true;
            break;
        }
        case OBJ_BULLET_ICE:
        case OBJ_BULLET_MISSILE:
        case OBJ_BULLET_XRAY:
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
int32_t rayEnemyNormalGetShotTimer(rayEnemy_t* enemy)
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
    return OBJ_BULLET_NORMAL;
}
