//==============================================================================
// Includes
//==============================================================================

#include "ray_script.h"
#include "ray_enemy.h"
#include "ray_enemy_box.h"
#include "ray_enemy_grunt.h"
#include "ray_enemy_turret.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool animateEnemy(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Create an enemy
 *
 * @param ray The entire game state
 * @param type The type of enemy to spawn
 * @param id The ID for this enemy
 * @param x The X position for this enemy
 * @param y The Y position for this enemy
 */
void rayCreateEnemy(ray_t* ray, rayMapCellType_t type, int32_t id, q24_8 x, q24_8 y)
{
    // Allocate the enemy
    rayEnemy_t* newObj = (rayEnemy_t*)heap_caps_calloc(1, sizeof(rayEnemy_t), MALLOC_CAP_SPIRAM);

    // Set type and ID first
    newObj->c.type = type;
    newObj->c.id   = id;

    switch (type)
    {
        case OBJ_ENEMY_BOX:
        {
            rayInitEnemyBox(ray, newObj);
            break;
        }
        case OBJ_ENEMY_GRUNT:
        {
            rayInitEnemyGrunt(ray, newObj);
            break;
        }
        case OBJ_ENEMY_TURRET:
        {
            rayInitEnemyTurret(ray, newObj);
            break;
        }
        default:
        {
            // Unknown enemy
            heap_caps_free(newObj);
            return;
        }
    }

    // Set initial common state
    newObj->c.posX        = x;
    newObj->c.posY        = y;
    newObj->c.bound.box.w = TO_FX_FRAC(newObj->c.sprite->w, CELL_SIZE);
    newObj->c.bound.box.h = TO_FX_FRAC(newObj->c.sprite->h, CELL_SIZE);
    // Don't set radius
    newObj->c.spriteMirrored = false;

    // Add it to the linked list
    push(&ray->enemies, newObj);
}

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

        if (animateEnemy(ray, enemy, elapsedUs))
        {
            // Enemy was killed
            checkScriptKill(ray, enemy->c.id, enemy->portrait);

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
                heap_caps_free(enemy->state);
            }
            heap_caps_free(enemy);

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
 * @brief Check if an item should be created after an enemy dies
 *
 * @param ray The entire game state
 * @param enemy The enemy which died
 * @return The item to drop, or EMPTY
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
 * @brief Check for collisions between the given enemy and player.
 * Sometimes collisions cause damage.
 * Sometimes the enemy is immovable and stops the player.
 * Sometimes the player pushes the enemy
 *
 * @param ray The entire game state
 * @param enemy The enemy to check for collisions with
 * @param player The player to check for collisions with
 * @param deltaX The X distance the player is trying to move. May be set to 0 if the enemy is immovable.
 * @param deltaY The Y distance the player is trying to move. May be set to 0 if the enemy is immovable.
 */
void rayEnemyCheckCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
{
    if (enemy->collisionFn)
    {
        enemy->collisionFn(ray, enemy, player, deltaX, deltaY);
    }
}

/**
 * @brief Standard function for heart and mpoint drops
 *
 * @param ray The entire game state
 * @return rayMapCellType_t The item to drop, or EMPTY
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
