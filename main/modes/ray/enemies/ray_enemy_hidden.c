#include "ray_enemy.h"
#include "ray_enemy_hidden.h"

/**
 * @brief Run timers for a hidden enemy, which include AI, and movement
 *
 * @param ray The entire game state
 * @param enemy The enemy to run timers for
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayEnemyHiddenMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Find the vector from the enemy to the player and normalize it
    q24_8 xDiff = SUB_FX(ray->p.posX, enemy->c.posX);
    q24_8 yDiff = SUB_FX(ray->p.posY, enemy->c.posY);
    fastNormVec(&xDiff, &yDiff);

    // Pick a new direction every s
    enemy->behaviorTimer += elapsedUs;
    if (enemy->behaviorTimer > 1000000)
    {
        enemy->behaviorTimer -= 1000000;

        // Randomize Strafe
        if (esp_random() % 2)
        {
            enemy->behavior = MOVE_STRAFE_R;
        }
        else
        {
            enemy->behavior = MOVE_STRAFE_L;
        }

        // Shoot at the player
        rayEnemyTransitionState(enemy, E_SHOOTING);
        // TODO spawn on some other frame
        rayCreateBullet(ray, OBJ_BULLET_NORMAL, enemy->c.posX, enemy->c.posY, xDiff, yDiff, false);
    }

// Player is 40000 * 6
#define SPEED_DENOM_RETREAT (int32_t)(40000 * 6)
#define SPEED_DENOM_STRAFE  (int32_t)(40000 * 16)

    q24_8 delX    = 0;
    q24_8 delY    = 0;
    q24_8 marginX = 0;
    q24_8 marginY = 0;

    // Try to stay a constant-ish distance away
    q24_8 xDist        = SUB_FX(ray->p.posX, enemy->c.posX);
    q24_8 yDist        = SUB_FX(ray->p.posY, enemy->c.posY);
    q24_8 distToPlayer = ADD_FX(MUL_FX(xDist, xDist), MUL_FX(yDist, yDist));
    if (distToPlayer < TO_FX(16))
    {
        delX -= (xDiff * (int32_t)(elapsedUs)) / SPEED_DENOM_RETREAT;
        delY -= (yDiff * (int32_t)(elapsedUs)) / SPEED_DENOM_RETREAT;
        marginX -= xDiff;
        marginY -= yDiff;
    }
    else if (distToPlayer > TO_FX(17))
    {
        delX += (xDiff * (int32_t)(elapsedUs)) / SPEED_DENOM_RETREAT;
        delY += (yDiff * (int32_t)(elapsedUs)) / SPEED_DENOM_RETREAT;
        marginX += xDiff;
        marginY += yDiff;
    }

    // Do some strafing
    if (MOVE_STRAFE_L == enemy->behavior)
    {
        delX += (yDiff * (int32_t)(elapsedUs)) / SPEED_DENOM_STRAFE;
        delY -= (xDiff * (int32_t)(elapsedUs)) / SPEED_DENOM_STRAFE;
        marginX += yDiff;
        marginY -= xDiff;
    }
    else
    {
        delX -= (yDiff * (int32_t)(elapsedUs)) / SPEED_DENOM_STRAFE;
        delY += (xDiff * (int32_t)(elapsedUs)) / SPEED_DENOM_STRAFE;
        marginX -= yDiff;
        marginY += xDiff;
    }

    // Scale the margin
    fastNormVec(&marginX, &marginY);
    marginX /= 2;
    marginY /= 2;

    // Move if in bounds
    if (isPassableCell(
            &ray->map.tiles[FROM_FX(enemy->c.posX + delX + marginX)][FROM_FX(enemy->c.posY + delY + marginY)]))
    {
        enemy->c.posX += delX;
        enemy->c.posY += delY;
    }
    else
    {
        // Could not move into wall, reverse the strafe
        if (MOVE_STRAFE_L == enemy->behavior)
        {
            enemy->behavior = MOVE_STRAFE_R;
        }
        else
        {
            enemy->behavior = MOVE_STRAFE_L;
        }
    }
}

/**
 * @brief This is called when a hidden enemy is shot. It adds damage based on bullet type, checks scripts, and handles
 * freeing defeated enemies
 *
 * @param ray The entire game state
 * @param enemy The enemy which was shot
 * @param bullet The type of bullet it was shot by
 * @return true if the enemy was killed, false if it's still alive
 */
bool rayEnemyHiddenGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // Health starts at 100
    bool hurt = false;
    switch (bullet)
    {
        case OBJ_BULLET_XRAY:
        {
            // Two shots to kill
            enemy->health -= 50;
            hurt = true;
            break;
        }
        case OBJ_BULLET_ICE:
        case OBJ_BULLET_NORMAL:
        case OBJ_BULLET_CHARGE:
        case OBJ_BULLET_MISSILE:
        default:
        {
            // Not a bullet or invulnerable
            break;
        }
    }

    if (hurt)
    {
        rayEnemyTransitionState(enemy, E_HURT);
    }
    return enemy->health <= 0;
}
