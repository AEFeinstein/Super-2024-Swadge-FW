#include "ray_enemy_hidden.h"

/**
 * @brief Run timers for a hidden enemy, which include AI, movement, and animation
 *
 * @param ray The entire game state
 * @param enemy The enemy to run timers for
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayEnemyHiddenMoveAnimate(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
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
    return false;
}
