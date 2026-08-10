//==============================================================================
// Includes
//==============================================================================

#include "ray_script.h"
#include "ray_enemy.h"
#include "ray_enemy_block.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool animateEnemy(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);

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

        // TODO Call function to move enemies, run timers, update state, pick current sprite
        if (animateEnemy(ray, enemy, elapsedUs))
        {
            // Enemy was killed
            // TODO function for portrait
            checkScriptKill(ray, enemy->c.id, NULL);

            // save the next node
            node_t* nextNode = currentNode->next;

            // TODO call function to randomly drop a pickup
            // switch (esp_random() % 4)
            // {
            //     case 0:
            //     {
            //         // 25% health first
            //         if (ray->p.i.health != ray->p.i.maxHealth)
            //         {
            //             rayCreateCommonObj(ray, OBJ_ITEM_PICKUP_ENERGY, 0x100, enemy->c.posX, enemy->c.posY);
            //         }
            //         // Only create if missiles are unlocked and health is at max
            //         else if ((ray->p.i.missileLoadOut) && (ray->p.i.numMissiles != ray->p.i.maxNumMissiles))
            //         {
            //             rayCreateCommonObj(ray, OBJ_ITEM_PICKUP_MISSILE, 0x100, enemy->c.posX, enemy->c.posY);
            //         }
            //         break;
            //     }
            //     case 1:
            //     {
            //         // 25% missiles first, if unlocked
            //         if ((ray->p.i.missileLoadOut) && (ray->p.i.numMissiles != ray->p.i.maxNumMissiles))
            //         {
            //             rayCreateCommonObj(ray, OBJ_ITEM_PICKUP_MISSILE, 0x100, enemy->c.posX, enemy->c.posY);
            //         }
            //         // Only create if missiles are at max
            //         else if (ray->p.i.health != ray->p.i.maxHealth)
            //         {
            //             rayCreateCommonObj(ray, OBJ_ITEM_PICKUP_ENERGY, 0x100, enemy->c.posX, enemy->c.posY);
            //         }
            //         break;
            //     }
            //     default:
            //     {
            //         // No drop
            //         break;
            //     }
            // }

            // Unlink and free
            removeEntry(&ray->enemies, currentNode);
            free(enemy);

            // Set the next node
            currentNode = nextNode;
        }
        else
        {
            // Iterate to the next node
            currentNode = currentNode->next;
        }
    }
}

/**
 * @brief This is called when an enemy is shot. It adds damage based on bullet type, checks scripts, and handles freeing
 * defeated enemies
 *
 * @param ray The entire game state
 * @param enemy The enemy which was shot
 * @param bullet The type of bullet it was shot by
 */
void rayEnemyGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // TODO call function when enemy is damaged
}

/**
 * @brief Animate a single enemy
 *
 * @param ray The entire game state
 * @param enemy The enemy to animate
 * @param elapsedUs The elapsed time since this function was last called
 * @return True if the enemy died, false if not
 */
static bool animateEnemy(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    return enemy->mainFn(ray, enemy, elapsedUs);
}

/**
 * @brief TODO doc
 *
 * @param ray
 * @param enemy
 * @param player
 * @param deltaX
 * @param deltaY
 */
void rayEnemyCheckCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
{
    enemy->collisionFn(ray, enemy, player, deltaX, deltaY);
}
