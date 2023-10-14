//==============================================================================
// Includes
//==============================================================================

#include "ray_script.h"
#include "ray_enemy.h"
#include "ray_enemy_normal.h"
#include "ray_enemy_strong.h"
#include "ray_enemy_armored.h"
#include "ray_enemy_flaming.h"
#include "ray_enemy_hidden.h"
#include "ray_enemy_boss.h"

//==============================================================================
// Const data
//==============================================================================

/**
 * @brief Functions to call for each enemy type
 * Must be in order OBJ_ENEMY_NORMAL -> OBJ_ENEMY_BOSS
 */
static const enemyFuncs_t enemyFuncs[] = {
    {
        // OBJ_ENEMY_NORMAL
        .moveAnimate = rayEnemyNormalMoveAnimate,
        .getShot     = rayEnemyNormalGetShot,
    },
    {
        // OBJ_ENEMY_STRONG
        .moveAnimate = rayEnemyStrongMoveAnimate,
        .getShot     = rayEnemyStrongGetShot,
    },
    {
        // OBJ_ENEMY_ARMORED
        .moveAnimate = rayEnemyArmoredMoveAnimate,
        .getShot     = rayEnemyArmoredGetShot,
    },
    {
        // OBJ_ENEMY_FLAMING
        .moveAnimate = rayEnemyFlamingMoveAnimate,
        .getShot     = rayEnemyFlamingGetShot,
    },
    {
        // OBJ_ENEMY_HIDDEN
        .moveAnimate = rayEnemyHiddenMoveAnimate,
        .getShot     = rayEnemyHiddenGetShot,
    },
    {
        // OBJ_ENEMY_BOSS
        .moveAnimate = rayEnemyBossMoveAnimate,
        .getShot     = rayEnemyBossGetShot,
    },
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Run timers for enemies, which include AI, movement, and animation
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayEnemiesMoveAnimate(ray_t* ray, uint32_t elapsedUs)
{
    // Iterate over the linked list
    node_t* currentNode = ray->enemies.first;
    while (currentNode != NULL)
    {
        // Get a pointer from the linked list
        rayEnemy_t* enemy = ((rayEnemy_t*)currentNode->val);

        // Move enemies and run timers like animation
        enemyFuncs[enemy->c.type - OBJ_ENEMY_NORMAL].moveAnimate(ray, enemy, elapsedUs);

        // Iterate to the next node
        currentNode = currentNode->next;
    }
}

/**
 * @brief This is called when an enemy is shot. It adds damage based on bullet type, checks scripts, and handles freeing
 * defeated enemies
 *
 * @param ray The entire game state
 * @param enemy The enemy which was shot
 * @param bullet The type of bullet it was shot by
 * @return true if the enemy was killed, false if it's still alive
 */
bool rayEnemyGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    return enemyFuncs[enemy->c.type - OBJ_ENEMY_NORMAL].getShot(ray, enemy, bullet);
}
