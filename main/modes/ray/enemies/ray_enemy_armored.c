#include "ray_enemy.h"
#include "ray_enemy_armored.h"

/**
 * @brief Run timers for an armored enemy, which include AI, and movement
 *
 * @param ray The entire game state
 * @param enemy The enemy to run timers for
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayEnemyArmoredMove(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Pick a new direction every 1s
    enemy->behaviorTimer += elapsedUs;
    if (enemy->behaviorTimer > 1000000)
    {
        enemy->behaviorTimer -= 1000000;

        // Find the vector from the enemy to the player and normalize it
        q24_8 xDiff = SUB_FX(ray->p.posX, enemy->c.posX);
        q24_8 yDiff = SUB_FX(ray->p.posY, enemy->c.posY);
        fastNormVec(&xDiff, &yDiff);

        // Pick the movement direction on the grid
        if (ABS(xDiff) > ABS(yDiff))
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

        // Shoot at the player
        rayEnemyTransitionState(enemy, E_SHOOTING);
        // TODO spawn on some other frame
        rayCreateBullet(ray, OBJ_BULLET_NORMAL, enemy->c.posX, enemy->c.posY, xDiff, yDiff, false);
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

    // Move if in bounds
    if (isPassableCell(&ray->map.tiles[FROM_FX(enemy->c.posX + delX)][FROM_FX(enemy->c.posY + delY)]))
    {
        enemy->c.posX += delX;
        enemy->c.posY += delY;
    }
}

/**
 * @brief This is called when an armored enemy is shot. It adds damage based on bullet type, checks scripts, and handles
 * freeing defeated enemies
 *
 * @param ray The entire game state
 * @param enemy The enemy which was shot
 * @param bullet The type of bullet it was shot by
 * @return true if the enemy was killed, false if it's still alive
 */
bool rayEnemyArmoredGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // Health starts at 100
    bool hurt = false;
    switch (bullet)
    {
        case OBJ_BULLET_MISSILE:
        {
            // Two shots to kill
            enemy->health -= 50;
            hurt = true;
            break;
        }
        case OBJ_BULLET_NORMAL:
        case OBJ_BULLET_CHARGE:
        case OBJ_BULLET_ICE:
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
