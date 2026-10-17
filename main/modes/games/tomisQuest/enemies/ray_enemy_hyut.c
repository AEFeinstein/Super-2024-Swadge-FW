//==============================================================================
// Includes
//==============================================================================

#include "ray_enemy.h"
#include "ray_enemy_hyut.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    vec_q24_8 dir;      ///< The direction the hyut is facing
    int32_t dirTimer;   ///< A timer to randomly pick new directions to face
    int32_t deathTimer; ///< A timer to die
} hyutState_t;

//==============================================================================
// Function Declarations
//==============================================================================

static bool rayEnemyHyutMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
static void rayEnemyHyutCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX,
                                             q24_8* deltaY);
static void rayEnemyHyutPickDirection(rayPlayer_t* player, rayEnemy_t* enemy);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a hyut
 *
 * @param ray The whole game state
 * @param e The hyut to initialize
 */
void rayInitEnemyHyut(ray_t* ray, rayEnemy_t* e)
{
    // Hyut has one texture, for now
    e->c.sprite = loadTexture(ray, OBJ_ENEMY_NPC_HYUT_WSG, OBJ_ENEMY_NPC_HYUT);
    e->portrait = e->c.sprite;

    // Hyut has 1hp and dies on a timer
    e->health = 1;

    // Allocate state for this enemy
    e->state = heap_caps_calloc(1, sizeof(hyutState_t), MALLOC_CAP_8BIT);

    // Set function pointers
    e->mainFn           = rayEnemyHyutMain;
    e->collisionFn      = rayEnemyHyutCheckPlayerCollision;
    e->getShotFn        = NULL; // Can't get shot
    e->dropAfterDeathFn = NULL; // No drops

    // Pick a random direction to start traveling
    rayEnemyHyutPickDirection(&ray->p, e);
}

/**
 * @brief Pick a new direction for the hyut to travel, adjusting direction and sprite rotation
 *
 * @param enemy The hyut which is picking a new direction to travel
 */
static void rayEnemyHyutPickDirection(rayPlayer_t* player, rayEnemy_t* enemy)
{
    hyutState_t* state = enemy->state;

    // Aim towards the player
    state->dir.x = player->posX - enemy->c.posX;
    state->dir.y = player->posY - enemy->c.posY;

    // Don't allow zero motion
    if (0 == state->dir.x && 0 == state->dir.y)
    {
        state->dir.x = esp_random();
        state->dir.y = esp_random();
    }

    fastNormVec(&state->dir.x, &state->dir.y);

    // Adjust the sprite to match the direction
    int32_t angle           = cordicAtan2(state->dir.x, state->dir.y);
    enemy->c.spriteRotation = (((angle + 23) / 45) * 45);
}

/**
 * @brief Main loop for the Hyut enemy. Handles movement logic.
 *
 * @param ray The whole game state
 * @param enemy The Hyut enemy
 * @param elapsedUs The time since this function was last called
 * @return true if this enemy is dead (after any death animations), false if it is alive
 */
bool rayEnemyHyutMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Casted pointer for convenience
    hyutState_t* state = enemy->state;

    // Return if dead for cleanup
    if (state->deathTimer >= 30 * 1000000)
    {
        return true;
    }
    else
    {
        // Increment death timer
        state->deathTimer += elapsedUs;

        // Change direction three seconds
        RUN_TIMER_EVERY(state->dirTimer, 3 * 1000000, elapsedUs, { rayEnemyHyutPickDirection(&ray->p, enemy); });

        // Move twice as fast as than the player
        vec_q24_8 delta = {
            .x = (state->dir.x * (int32_t)elapsedUs) / (40000 * 3),
            .y = (state->dir.y * (int32_t)elapsedUs) / (40000 * 3),
        };

        // Get a bounding box for where the enemy is
        rectangle_t bb = rayGetObjBB(&enemy->c);

        // Move the bounding box without moving the enemy yet
        bb.pos.x += delta.x;
        bb.pos.y += delta.y;

        // Update the enemy position if it's in bounds
        vec_t collision;
        if (rayBoundingBoxFitsInMap(ray, bb, &collision))
        {
            enemy->c.posX += delta.x;
            enemy->c.posY += delta.y;
        }
        else
        {
            // Can't move this way, bounce off the wall
            if (collision.x)
            {
                state->dir.x *= -1;
            }
            if (collision.y)
            {
                state->dir.y *= -1;
            }

            int32_t angle           = cordicAtan2(state->dir.x, state->dir.y);
            enemy->c.spriteRotation = (((angle + 23) / 45) * 45);
        }

        // Not dead yet!
        return false;
    }
}

/**
 * @brief Function to check if the hyut collides with the player
 * Touching the hyut damages and bumps the player
 *
 * @param ray The whole game state
 * @param enemy The hyut to check for collisions
 * @param player The player to check for collisions
 * @param deltaX The X distance the player is trying to move
 * @param deltaY The Y distance the player is trying to move
 */
void rayEnemyHyutCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
{
    // If the player is currently hittable
    if (rayPlayerIsHittable(ray))
    {
        // Check if there's a bounding box intersection
        rectangle_t enemyBB = rayGetObjBB(&enemy->c);
        if (rectRectIntersection(player, enemyBB, NULL))
        {
            // Damage the player
            if (rayPlayerDecrementHealth(ray, 1))
            {
                // Give the player a bump away from the hyut
                ray->ps.vel.x = ray->p.posX - enemy->c.posX;
                ray->ps.vel.y = ray->p.posY - enemy->c.posY;
                fastNormVec(&ray->ps.vel.x, &ray->ps.vel.y);

                // Bump for 250ms
                ray->ps.bumpTimer = 250000;
            }
        }
    }
}
