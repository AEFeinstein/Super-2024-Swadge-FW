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
        .move    = rayEnemyNormalMove,
        .getShot = rayEnemyNormalGetShot,
    },
    {
        // OBJ_ENEMY_STRONG
        .move    = rayEnemyStrongMove,
        .getShot = rayEnemyStrongGetShot,
    },
    {
        // OBJ_ENEMY_ARMORED
        .move    = rayEnemyArmoredMove,
        .getShot = rayEnemyArmoredGetShot,
    },
    {
        // OBJ_ENEMY_FLAMING
        .move    = rayEnemyFlamingMove,
        .getShot = rayEnemyFlamingGetShot,
    },
    {
        // OBJ_ENEMY_HIDDEN
        .move    = rayEnemyHiddenMove,
        .getShot = rayEnemyHiddenGetShot,
    },
    {
        // OBJ_ENEMY_BOSS
        .move    = rayEnemyBossMove,
        .getShot = rayEnemyBossGetShot,
    },
};

//==============================================================================
// Function Prototypes
//==============================================================================

static bool animateEnemy(rayEnemy_t* enemy, uint32_t elapsedUs);

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

        // Move enemies and run timers if not dying
        if (E_DEAD != enemy->state)
        {
            enemyFuncs[enemy->c.type - OBJ_ENEMY_NORMAL].move(ray, enemy, elapsedUs);
        }

        // Animate the enemy
        if (animateEnemy(enemy, elapsedUs))
        {
            // Enemy was killed
            checkScriptKill(ray, enemy->c.id, &enemy->sprites[0][E_WALKING][0]);

            // save the next node
            node_t* nextNode = currentNode->next;

            // Remove the lock
            if (ray->targetedObj == &(enemy->c))
            {
                ray->targetedObj = NULL;
            }

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
    if (enemyFuncs[enemy->c.type - OBJ_ENEMY_NORMAL].getShot(ray, enemy, bullet))
    {
        // Transition to dying
        rayEnemyTransitionState(enemy, E_DEAD);
    }
}

/**
 * @brief Transition from the current animation state to the next
 *
 * @param enemy The enemy to transition
 * @param newState The new state to transition to
 */
void rayEnemyTransitionState(rayEnemy_t* enemy, rayEnemyState_t newState)
{
    if (E_DEAD == enemy->state)
    {
        // Never transition away from the death state
        return;
    }
    else
    {
        // Switch to the new state
        enemy->state          = newState;
        enemy->animTimer      = 0;
        enemy->animFrame      = 0;
        enemy->animTimerLimit = 250000;
        // Pick the sprites
        enemy->c.sprite = &enemy->sprites[0][newState][0];
        if (E_SHOOTING == newState)
        {
            enemy->animTimerLimit = 100000;
        }
    }
}

/**
 * @brief Animate a single enemy
 *
 * @param enemy The enemy to animate
 * @param elapsedUs The elapsed time since this function was last called
 * @return True if the enemy died, false if not
 */
static bool animateEnemy(rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Accumulate time
    enemy->animTimer += elapsedUs;
    // Check if it's time to transition states
    if (enemy->animTimer >= enemy->animTimerLimit)
    {
        // Decrement timer
        enemy->animTimer -= enemy->animTimerLimit;

        // Pick the sprites and limits based on the state
        int32_t limit = NUM_NON_WALK_FRAMES;
        if (E_WALKING == enemy->state)
        {
            limit = NUM_WALK_FRAMES;
        }

        // Increment to the next frame
        enemy->animFrame++;
        // If the animation is over
        if (enemy->animFrame >= limit)
        {
            if (E_DEAD == enemy->state)
            {
                // Dead, caller unlinks and frees the enemy
                return true;
            }
            else
            {
                // Return to walking
                rayEnemyTransitionState(enemy, E_WALKING);
            }
        }
        else
        {
            // Not past the limit, but If the frame is past the number of frames
            if (enemy->animFrame >= NUM_NON_WALK_FRAMES)
            {
                // Mirror it and start over
                enemy->c.spriteMirrored = !enemy->c.spriteMirrored;
                enemy->animFrame        = 0;
            }
            // Pick the next sprite
            enemy->c.sprite = &enemy->sprites[0][enemy->state][enemy->animFrame];
        }
    }
    return false;
}

/**
 * @brief Switch sprites for all enemies to the X-Ray alternate
 *
 * @param ray The entire game state
 * @param isXray true to switch to X-Ray sprites, false to switch to normal sprites
 */
void switchEnemiesToXray(ray_t* ray, bool isXray)
{
    // Assign each enemy a distance from the player
    node_t* currentNode = ray->enemies.first;
    while (currentNode != NULL)
    {
        // Get a pointer from the linked list
        rayEnemy_t* enemy = ((rayEnemy_t*)currentNode->val);

        // If this is a hidden enemy
        if (OBJ_ENEMY_HIDDEN == enemy->c.type)
        {
            // Switch all the sprites
            if (isXray)
            {
                enemy->sprites = &ray->hiddenXRTex;
            }
            else
            {
                enemy->sprites = &ray->enemyTex[OBJ_ENEMY_HIDDEN - OBJ_ENEMY_NORMAL];
            }

            // Switch the current sprite
            enemy->c.sprite = &enemy->sprites[0][enemy->state][enemy->animFrame];
        }

        // Iterate to the next node
        currentNode = currentNode->next;
    }
}