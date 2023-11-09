#include "ray_enemy.h"
#include "ray_enemy_strong.h"

/**
 * @brief Run timers for a strong enemy, which include AI, and movement
 *
 * @param ray The entire game state
 * @param enemy The enemy to run timers for
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayEnemyStrongMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Pick a new direction every 2s
    enemy->behaviorTimer -= elapsedUs;
    if (enemy->behaviorTimer <= 0)
    {
        enemy->behaviorTimer += 2000000;

        // Randomize movement
        switch (esp_random() % 8)
        {
            case 0:
            {
                enemy->behavior = MOVE_AWAY_PLAYER;
                break;
            }
            case 1 ... 2:
            {
                enemy->behavior = MOVE_STRAFE_R;
                break;
            }
            case 3 ... 4:
            {
                enemy->behavior = MOVE_STRAFE_L;
                break;
            }
            case 5 ... 7:
            {
                enemy->behavior = MOVE_TOWARDS_PLAYER;
                break;
            }
        }
    }

    // Reverse behavior if too close to the player
    q24_8 xDist        = SUB_FX(ray->p.posX, enemy->c.posX);
    q24_8 yDist        = SUB_FX(ray->p.posY, enemy->c.posY);
    q24_8 distToPlayer = ADD_FX(MUL_FX(xDist, xDist), MUL_FX(yDist, yDist));
    if (distToPlayer < TO_FX(4) && (MOVE_TOWARDS_PLAYER == enemy->behavior))
    {
        enemy->behavior = MOVE_AWAY_PLAYER;
    }

    // Find the vector from the enemy to the player and normalize it
    q24_8 xDiff = SUB_FX(ray->p.posX, enemy->c.posX);
    q24_8 yDiff = SUB_FX(ray->p.posY, enemy->c.posY);
    fastNormVec(&xDiff, &yDiff);

// Player is 40000 * 6
#define SPEED_DENOM (int32_t)(40000 * 18)

    q24_8 delX = 0;
    q24_8 delY = 0;
    switch (enemy->behavior)
    {
        case MOVE_AWAY_PLAYER:
        {
            delX = -(xDiff * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY = -(yDiff * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        case MOVE_TOWARDS_PLAYER:
        {
            delX = (xDiff * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY = (yDiff * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        case MOVE_STRAFE_L:
        {
            delX = (yDiff * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY = -(xDiff * (int32_t)(elapsedUs)) / SPEED_DENOM;
            break;
        }
        case MOVE_STRAFE_R:
        {
            delX = -(yDiff * (int32_t)(elapsedUs)) / SPEED_DENOM;
            delY = (xDiff * (int32_t)(elapsedUs)) / SPEED_DENOM;
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
 * @brief This is called when a strong enemy is shot. It adds damage based on bullet type, checks scripts, and handles
 * freeing defeated enemies
 *
 * @param ray The entire game state
 * @param enemy The enemy which was shot
 * @param bullet The type of bullet it was shot by
 * @param damageDivide A divisor for damage
 * @return true if the enemy was killed, false if it's still alive
 */
bool rayEnemyStrongGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet, int32_t damageDivide)
{
    // Health starts at 100
    int32_t damage = 0;
    switch (bullet)
    {
        case OBJ_BULLET_NORMAL:
        {
            // Invincible to normal bullets
            break;
        }
        case OBJ_BULLET_CHARGE:
        {
            // One shot, one kill
            damage = 100;
            break;
        }
        case OBJ_BULLET_ICE:
        case OBJ_BULLET_MISSILE:
        case OBJ_BULLET_XRAY:
        {
            // Five shots to kill
            damage = 20;
            break;
        }
        default:
        {
            // Not a bullet
            break;
        }
    }

    enemy->health -= ((damage * ray->p.i.damageMult) / damageDivide);
    return enemy->health <= 0;
}

/**
 * @brief Get the time until the next shot is taken
 *
 * @param enemy The enemy taking the shot
 * @param type the timer of timer to get
 * @return The time, in uS, until the next shot
 */
int32_t rayEnemyStrongGetTimer(rayEnemy_t* enemy, rayEnemyTimerType_t type)
{
    return 2000000 + (esp_random() % 2000000);
}

/**
 * @brief Get the bullet this enemy fires
 *
 * @param enemy The shooting enemy
 * @return The bullet type
 */
rayMapCellType_t rayEnemyStrongGetBullet(rayEnemy_t* enemy)
{
    return OBJ_BULLET_E_STRONG;
}
