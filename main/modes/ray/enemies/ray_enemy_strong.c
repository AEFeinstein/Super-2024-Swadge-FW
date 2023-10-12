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
}

/**
 * @brief This is called when a strong enemy is shot. It adds damage based on bullet type, checks scripts, and handles
 * freeing defeated enemies
 *
 * @param ray The entire game state
 * @param enemy The enemy which was shot
 * @param bullet The type of bullet it was shot by
 * @return true if the enemy was killed, false if it's still alive
 */
bool rayEnemyStrongGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    if (OBJ_BULLET_CHARGE == bullet)
    {
        return true;
    }
    return false;
}
