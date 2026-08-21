//==============================================================================
// Includes
//==============================================================================

#include "ray_script.h"
#include "ray_enemy.h"

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

            // Maybe create an item after death
            rayMapCellType_t dropType = enemyDropAfterDeath(enemy);
            switch (dropType)
            {
                case OBJ_ITEM_EWI ... OBJ_ITEM_31:
                {
                    rayCreateCommonObj(ray, dropType, 0x100, enemy->c.posX, enemy->c.posY);
                    break;
                }
                default:
                {
                    break;
                }
            }

            // Unlink and free
            removeEntry(&ray->enemies, currentNode);
            if (enemy->state)
            {
                free(enemy->state);
            }
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
    if (enemy->getShotFn)
    {
        enemy->getShotFn(ray, enemy, bullet);
    }
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
    if (enemy->mainFn)
    {
        return enemy->mainFn(ray, enemy, elapsedUs);
    }
    return false;
}

/**
 * @brief TODO doc
 *
 * @param ray
 * @param enemy
 * @return rayMapCellType_t
 */
rayMapCellType_t enemyDropAfterDeath(rayEnemy_t* enemy)
{
    if (enemy->dropAfterDeathFn)
    {
        return enemy->dropAfterDeathFn();
    }
    return EMPTY;
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
    if (enemy->collisionFn)
    {
        enemy->collisionFn(ray, enemy, player, deltaX, deltaY);
    }
}

/**
 * @brief
 *
 * @param ray
 * @param enemy
 * @return rayMapCellType_t
 */
rayMapCellType_t rayEnemyStandardItemDrop(void)
{
    switch ((uint8_t)(esp_random() & 0xFF))
    {
        case 0 ... 46:
        {
            // 18.5% chance of 1 mpoint
            return OBJ_ITEM_MPOINT_1;
        }
        case 47 ... 55:
        {
            // 3.7% chance of 5 mpoint
            return OBJ_ITEM_MPOINT_5;
        }
        case 56 ... 60:
        {
            // 1.9% chance of 10 mpoint
            return OBJ_ITEM_MPOINT_10;
        }
        case 61 ... 62:
        {
            // 0.9% chance of 20 mpoint
            return OBJ_ITEM_MPOINT_20;
        }
        case 63 ... 127:
        {
            // 25% chance of heart
            return OBJ_ITEM_HEART;
        }
        default:
        {
            // 50% chance of nothing
            return EMPTY;
        }
    }
}
