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
// Typedefs
//==============================================================================

typedef void (*rayEnemyMove_t)(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
typedef bool (*rayEnemyGetShot_t)(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet, int32_t damageDivide);
typedef int32_t (*rayEnemyGetTimer_t)(rayEnemy_t* enemy, rayEnemyTimerType_t type);
typedef rayMapCellType_t (*rayEnemyGetBullet_t)(rayEnemy_t* enemy);

typedef struct ray_enemy
{
    rayEnemyMove_t move;
    rayEnemyGetShot_t getShot;
    rayEnemyGetTimer_t getTimer;
    rayEnemyGetBullet_t getBullet;
} enemyFuncs_t;

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
        .move      = rayEnemyNormalMove,
        .getShot   = rayEnemyNormalGetShot,
        .getTimer  = rayEnemyNormalGetTimer,
        .getBullet = rayEnemyNormalGetBullet,
    },
    {
        // OBJ_ENEMY_STRONG
        .move      = rayEnemyStrongMove,
        .getShot   = rayEnemyStrongGetShot,
        .getTimer  = rayEnemyStrongGetTimer,
        .getBullet = rayEnemyStrongGetBullet,
    },
    {
        // OBJ_ENEMY_ARMORED
        .move      = rayEnemyArmoredMove,
        .getShot   = rayEnemyArmoredGetShot,
        .getTimer  = rayEnemyArmoredGetTimer,
        .getBullet = rayEnemyArmoredGetBullet,
    },
    {
        // OBJ_ENEMY_FLAMING
        .move      = rayEnemyFlamingMove,
        .getShot   = rayEnemyFlamingGetShot,
        .getTimer  = rayEnemyFlamingGetTimer,
        .getBullet = rayEnemyFlamingGetBullet,
    },
    {
        // OBJ_ENEMY_HIDDEN
        .move      = rayEnemyHiddenMove,
        .getShot   = rayEnemyHiddenGetShot,
        .getTimer  = rayEnemyHiddenGetTimer,
        .getBullet = rayEnemyHiddenGetBullet,
    },
    {
        // OBJ_ENEMY_BOSS
        .move      = rayEnemyBossMove,
        .getShot   = rayEnemyBossGetShot,
        .getTimer  = rayEnemyBossGetTimer,
        .getBullet = rayEnemyBossGetBullet,
    },
};

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

        // If this enemy is warping in
        if (0 < enemy->warpTimer)
        {
            // Run down the warp timer
            enemy->warpTimer -= elapsedUs;
            // Iterate to the next node and continue
            currentNode = currentNode->next;
            continue;
        }

        // Copy the elapsed time in case the enemy is frozen
        uint32_t eElapsedUs = elapsedUs;

        // If the enemy is frozen
        if (0 < enemy->freezeTimer)
        {
            // Decrement the timer
            enemy->freezeTimer -= eElapsedUs;
            // Slow time for other timers
            eElapsedUs /= 3;
        }

        // Move enemies in the walking state, or if it's a non-dead boss
        if ((E_WALKING_1 == enemy->state) || (E_WALKING_2 == enemy->state)
            || ((OBJ_ENEMY_BOSS == enemy->c.type) && (E_DEAD != enemy->state)))
        {
            enemyFuncs[enemy->c.type - OBJ_ENEMY_NORMAL].move(ray, enemy, eElapsedUs);
        }

        // Run other timers if not dead
        if (E_DEAD != enemy->state)
        {
            // Run the shot timer down to zero
            enemy->shootTimer -= eElapsedUs;
            if (enemy->shootTimer <= 0)
            {
                // Try to transition to shooting
                if (rayEnemyTransitionState(enemy, E_SHOOTING))
                {
                    // If successful, restart the shot timer
                    enemy->shootTimer = enemyFuncs[enemy->c.type - OBJ_ENEMY_NORMAL].getTimer(enemy, SHOT);
                }
            }

            // Run the block timer down to zero
            enemy->blockTimer -= eElapsedUs;
            if (enemy->blockTimer <= 0)
            {
                // Try to transition to blocking
                if (rayEnemyTransitionState(enemy, E_BLOCKING))
                {
                    // If successful, restart the block timer
                    enemy->blockTimer = enemyFuncs[enemy->c.type - OBJ_ENEMY_NORMAL].getTimer(enemy, BLOCK);
                }
            }

            if (OBJ_ENEMY_BOSS == enemy->c.type)
            {
                // Special boss timer, pick a new weakness every 4s
                enemy->bossTimer -= elapsedUs;
                if (enemy->bossTimer <= 0)
                {
                    enemy->bossTimer += 4000000;

                    // Pick a new boss state
                    enemy->bossState
                        = (enemy->bossState + (1 + esp_random() % (NUM_BOSS_STATES - 1))) % NUM_BOSS_STATES;

                    // Update all sprites the boss uses
                    if (B_NORMAL == enemy->bossState)
                    {
                        enemy->sprites = &ray->enemyTex[OBJ_ENEMY_BOSS - OBJ_ENEMY_NORMAL];
                    }
                    else
                    {
                        enemy->sprites = &ray->bossTex[enemy->bossState];
                    }
                    // Update the current sprite
                    enemy->c.sprite = &enemy->sprites[0][enemy->state][enemy->animFrame];
                }
            }

            // Run the invincible timer down to zero
            if (0 < enemy->invincibleTimer)
            {
                enemy->invincibleTimer -= eElapsedUs;
            }
        }

        // Animate the enemy
        if (animateEnemy(ray, enemy, eElapsedUs))
        {
            // Enemy was killed
            checkScriptKill(ray, enemy->c.id, &enemy->sprites[0][E_WALKING_1][0]);

            // save the next node
            node_t* nextNode = currentNode->next;

            // Remove the lock
            if (ray->targetedObj == &(enemy->c))
            {
                ray->targetedObj = NULL;
            }

            // Randomly drop a pickup where the enemy was
            switch (esp_random() % 4)
            {
                case 0:
                {
                    // 25% health first
                    if (ray->p.i.health != ray->p.i.maxHealth)
                    {
                        rayCreateCommonObj(ray, OBJ_ITEM_PICKUP_ENERGY, 0x100, enemy->c.posX, enemy->c.posY);
                    }
                    // Only create if missiles are unlocked and health is at max
                    else if ((ray->p.i.missileLoadOut) && (ray->p.i.numMissiles != ray->p.i.maxNumMissiles))
                    {
                        rayCreateCommonObj(ray, OBJ_ITEM_PICKUP_MISSILE, 0x100, enemy->c.posX, enemy->c.posY);
                    }
                    break;
                }
                case 1:
                {
                    // 25% missiles first, if unlocked
                    if ((ray->p.i.missileLoadOut) && (ray->p.i.numMissiles != ray->p.i.maxNumMissiles))
                    {
                        rayCreateCommonObj(ray, OBJ_ITEM_PICKUP_MISSILE, 0x100, enemy->c.posX, enemy->c.posY);
                    }
                    // Only create if missiles are at max
                    else if (ray->p.i.health != ray->p.i.maxHealth)
                    {
                        rayCreateCommonObj(ray, OBJ_ITEM_PICKUP_ENERGY, 0x100, enemy->c.posX, enemy->c.posY);
                    }
                    break;
                }
                default:
                {
                    // No drop
                    break;
                }
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
 * @brief Get the next shot timer for an enemy
 *
 * @param enemy The shooting enemy
 * @return int32_t The time in uS until the next shot
 */
int32_t getTimerForEnemy(rayEnemy_t* enemy, rayEnemyTimerType_t type)
{
    return enemyFuncs[enemy->c.type - OBJ_ENEMY_NORMAL].getTimer(enemy, type);
}

/**
 * @brief Get the bullet type for an enemy
 *
 * @param enemy The shooting enemy
 * @return The bullet type
 */
rayMapCellType_t getBulletForEnemy(rayEnemy_t* enemy)
{
    return enemyFuncs[enemy->c.type - OBJ_ENEMY_NORMAL].getBullet(enemy);
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
    int32_t damageDivide = 1;
    // Don't get shot when blocking or dying
    if (E_BLOCKING == enemy->state)
    {
        if (OBJ_BULLET_MISSILE != bullet)
        {
            // Play SFX
            bzrPlaySfx(&ray->sfx_e_block, BZR_RIGHT);
            return;
        }
        else
        {
            // Missiles break the block, but do half damage
            damageDivide = 2;
            rayEnemyTransitionState(enemy, E_WALKING_1);
        }
    }
    else if (E_DEAD == enemy->state)
    {
        // No SFX
        return;
    }

    // Don't get shot when invincible
    // Ignore this for now, being invincible when in E_HURT is enough
    // if (0 < enemy->invincibleTimer)
    // {
    //     return;
    // }

    // Save old health to see if the enemy took damage
    int32_t oldHealth = enemy->health;

    // Apply damage
    if (enemyFuncs[enemy->c.type - OBJ_ENEMY_NORMAL].getShot(ray, enemy, bullet, damageDivide))
    {
        // Transition to dying
        rayEnemyTransitionState(enemy, E_DEAD);
        // Play SFX
        bzrPlaySfx(&ray->sfx_e_dead, BZR_RIGHT);

        // If the boss died
        if (OBJ_ENEMY_BOSS == enemy->c.type)
        {
            // Resume normal music
            bzrPlayBgm(&ray->songs[ray->p.mapId], BZR_STEREO);
        }
    }
    // If the enemy took
    else if (oldHealth != enemy->health)
    {
        // Transition to hurt
        rayEnemyTransitionState(enemy, E_HURT);

        // Don't get stun-locked. Hurt animation is 200000*4, so double that
        // enemy->invincibleTimer = 200000 * 8;

        // If it was ice damage
        if (OBJ_BULLET_ICE == bullet)
        {
            // Slow it for a moment
            enemy->freezeTimer = 3200000;
            // Play SFX
            bzrPlaySfx(&ray->sfx_e_freeze, BZR_RIGHT);
        }
        else
        {
            // Play SFX
            bzrPlaySfx(&ray->sfx_e_damage, BZR_RIGHT);
        }
    }
}

/**
 * @brief Transition from the current animation state to the next
 *
 * @param enemy The enemy to transition
 * @param newState The new state to transition to
 * @return true if the transition was allowed, false if it was not
 */
bool rayEnemyTransitionState(rayEnemy_t* enemy, rayEnemyState_t newState)
{
    // clang-format off
    const bool transitionTable[E_NUM_STATES][E_NUM_STATES] = {
        {false, true,  true,  true,  true,  true }, // E_WALKING_1, don't transition to self
        {true,  false, true,  true,  true,  true }, // E_WALKING_2, don't transition to self
        {true,  true,  false, true , false, true }, // E_SHOOTING, transition to E_WALKING or E_HURT
        {true,  true,  false, false, false, true }, // E_HURT, transition to E_WALKING or E_DEAD
        {true,  true,  false, false, false, false}, // E_BLOCKING, transition to E_WALKING
        {false, false, false, false, false, false}  // E_DEAD, don't transition to anything
    };
    // clang-format on

    if (transitionTable[enemy->state][newState])
    {
        // Switch to the new state
        enemy->state          = newState;
        enemy->animTimer      = 0;
        enemy->animFrame      = 0;
        enemy->animTimerLimit = 200000;
        // Pick the sprites
        enemy->c.sprite = &enemy->sprites[0][newState][0];
        if ((E_WALKING_1 == newState) || (E_WALKING_2 == newState))
        {
            enemy->animTimerLimit = 125000;
        }
        return true;
    }
    return false;
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
    // Accumulate time
    enemy->animTimer += elapsedUs;
    // Check if it's time to transition states
    if (enemy->animTimer >= enemy->animTimerLimit)
    {
        // Decrement timer
        enemy->animTimer -= enemy->animTimerLimit;

        // Increment to the next frame
        enemy->animFrame++;

        // If the animation is over
        if (enemy->animFrame >= NUM_ANIM_FRAMES)
        {
            if (E_DEAD == enemy->state)
            {
                // Dead, caller unlinks and frees the enemy
                return true;
            }
            else
            {
                if (E_WALKING_1 == enemy->state)
                {
                    // Return to walking
                    rayEnemyTransitionState(enemy, E_WALKING_2);
                }
                else
                {
                    // Return to walking
                    rayEnemyTransitionState(enemy, E_WALKING_1);
                }
            }
        }
        else
        {
            // Pick the next sprite
            enemy->c.sprite = &enemy->sprites[0][enemy->state][enemy->animFrame];

            // If this is the 3rd frame of shooting
            if ((E_SHOOTING == enemy->state) && (2 == enemy->animFrame))
            {
                // Spawn a bullet
                q24_8 xDiff = SUB_FX(ray->p.posX, enemy->c.posX);
                q24_8 yDiff = SUB_FX(ray->p.posY, enemy->c.posY);
                fastNormVec(&xDiff, &yDiff);
                rayCreateBullet(ray, getBulletForEnemy(enemy), enemy->c.posX, enemy->c.posY, xDiff, yDiff, false);
            }
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