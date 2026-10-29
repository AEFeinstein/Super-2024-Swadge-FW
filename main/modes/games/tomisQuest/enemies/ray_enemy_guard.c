//==============================================================================
// Includes
//==============================================================================

#include "ray_enemy.h"
#include "ray_enemy_guard.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    TQ_EN_GUARD_ST_WATCHING,
    TQ_EN_GUARD_ST_CHASING,
    TQ_EN_GUARD_ST_BUMPED
} guardStateIndex_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    guardStateIndex_t state;
    int32_t stateTimer;
    vec_q24_8 dir;     ///< The direction the guard is facing
    vec_q24_8 vel; ///< The direction the guard got bumped
} guardState_t;

//==============================================================================
// Function Declarations
//==============================================================================

static bool rayEnemyGuardMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
static void rayEnemyGuardCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX,
                                              q24_8* deltaY);
static void rayEnemyGuardGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
static void rayEnemyGuardPickDirection(rayEnemy_t* enemy);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a guard
 *
 * @param ray The whole game state
 * @param e The guard to initialize
 */
void rayInitEnemyGuard(ray_t* ray, rayEnemy_t* e)
{
    // Guard has one texture, for now
    e->c.sprite = loadTexture(ray, OBJ_ENEMY_GRUNT_WSG, OBJ_ENEMY_22);
    e->portrait = e->c.sprite;

    // Guard has 1hp to not die
    e->health = 3;

    // Allocate state for this enemy
    e->state = heap_caps_calloc(1, sizeof(guardState_t), MALLOC_CAP_8BIT);

    ((guardState_t*) e->state)->state = TQ_EN_GUARD_ST_WATCHING;
    ((guardState_t*) e->state)->stateTimer = -1;

    // Set function pointers
    e->mainFn           = rayEnemyGuardMain;
    e->collisionFn      = rayEnemyGuardCheckPlayerCollision;
    e->getShotFn        = rayEnemyGuardGetShot;
    e->dropAfterDeathFn = rayEnemyStandardItemDrop;

    // Pick a random direction to start watching
    rayEnemyGuardPickDirection(e);
}

/**
 * @brief Pick a new direction for the guard to travel, adjusting direction and sprite rotation
 *
 * @param enemy The guard which is picking a new direction to travel
 */
static void rayEnemyGuardPickDirection(rayEnemy_t* enemy)
{
    guardState_t* state = enemy->state;

    // Pick a random cardinal direction
    state->dir.x            = 0;
    state->dir.y            = 0;
    enemy->c.spriteRotation = 0;
    switch (esp_random() & 0x03)
    {
        case 0x00:
        {
            state->dir.x            = TO_FX_FRAC(1, 2);
            enemy->c.spriteRotation = 90;
            break;
        }
        case 0x01:
        {
            state->dir.x            = -TO_FX_FRAC(1, 2);
            enemy->c.spriteRotation = 270;
            break;
        }
        case 0x02:
        {
            state->dir.y            = TO_FX_FRAC(1, 2);
            enemy->c.spriteRotation = 180;
            break;
        }
        case 0x03:
        {
            state->dir.y            = -TO_FX_FRAC(1, 2);
            enemy->c.spriteRotation = 0;
            break;
        }
    }
}

/**
 * @brief Main loop for the Guard enemy. Handles movement logic.
 *
 * @param ray The whole game state
 * @param enemy The Guard enemy
 * @param elapsedUs The time since this function was last called
 * @return true if this enemy is dead (after any death animations), false if it is alive
 */
bool rayEnemyGuardMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Return if dead for cleanup
    if (enemy->health <= 0)
    {
        return true;
    }

    // Casted pointer for convenience
    guardState_t* state = enemy->state;

    switch(state->state)
    {
        case TQ_EN_GUARD_ST_WATCHING:
        default:
        {
             // Change direction every second
            RUN_TIMER_EVERY(state->stateTimer, 1000000, elapsedUs, { rayEnemyGuardPickDirection(enemy); });

            state->vel.x = 0;
            state->vel.y = 0;

            vec_t enemyPos = {.x = enemy->c.posX, .y = enemy->c.posY};
            vec_t direction = {.x = state->dir.x, .y = state->dir.y};

            line_t sightLine = {
                .p1 = enemyPos,
                .p2 = addVec2d(enemyPos, mulVec2d(direction, 4 ) )
            };

            if(rectLineIntersection(rayGetPlayerBB(ray), sightLine, NULL))
            {
                state->state = TQ_EN_GUARD_ST_CHASING;
                state->stateTimer = 300;
            }
            
            break;
        }
        case TQ_EN_GUARD_ST_CHASING:
        {
            state->vel.x = (ray->p.posX - enemy->c.posX) < 0 ? -100 : 100;
            state->vel.y = (ray->p.posY - enemy->c.posY) < 0 ? -100 : 100;
            
            state->stateTimer--;
            if(state->stateTimer < 0)
            {
                state->state = TQ_EN_GUARD_ST_WATCHING;
            }

            break;
        }
        case TQ_EN_GUARD_ST_BUMPED:
        {
            state->stateTimer -= elapsedUs;

            if(state->stateTimer <= 0)
            {
                state->state = TQ_EN_GUARD_ST_CHASING;
                state->stateTimer = 300;
            }
            break;
        }
    }
    
    //Move half as fast as than the player
    vec_q24_8 delta = {
        .x = (state->vel.x * (int32_t)elapsedUs) / (40000 * 6),
        .y = (state->vel.y * (int32_t)elapsedUs) / (40000 * 6),
    };

    // Get a bounding box for where the enemy is
    rectangle_t bb = rayGetObjBB(&enemy->c);

    // Move the bounding box without moving the enemy yet
    bb.pos.x += delta.x;
    bb.pos.y += delta.y;

    // Update the enemy position if it's in bounds
    if (rayBoundingBoxFitsInMap(ray, bb, NULL))
    {
        enemy->c.posX += delta.x;
        enemy->c.posY += delta.y;
    }
    else
    {
        // Can't move this way, pick a new direction
        rayEnemyGuardPickDirection(enemy);
        state->stateTimer = 0;
    }

    // Not dead yet!
    return false;
}

/**
 * @brief Function to check if the guard collides with the player
 * Touching the guard damages and bumps the player
 *
 * @param ray The whole game state
 * @param enemy The guard to check for collisions
 * @param player The player to check for collisions
 * @param deltaX The X distance the player is trying to move
 * @param deltaY The Y distance the player is trying to move
 */
void rayEnemyGuardCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
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
                // Give the player a bump away from the guard
                ray->ps.vel.x = ray->p.posX - enemy->c.posX;
                ray->ps.vel.y = ray->p.posY - enemy->c.posY;
                fastNormVec(&ray->ps.vel.x, &ray->ps.vel.y);

                // Bump for 250ms
                ray->ps.bumpTimer = 250000;
            }
        }
    }
}

/**
 * @brief This function is called when the guard is hit
 *
 * @param ray The whole game state
 * @param enemy The enemy which was shot
 * @param bullet OBJ_BULLET_ARROW, OBJ_BULLET_BOMB, OBJ_BULLET_BOOMERANG, or OBJ_BULLET_SWORD
 */
void rayEnemyGuardGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // Convenience pointer
    guardState_t* state = enemy->state;

    switch (bullet)
    {
        case OBJ_BULLET_SWORD:
        {
            // Give the player a bump away from the guard
            state->vel.x = enemy->c.posX - ray->p.posX;
            state->vel.y = enemy->c.posY - ray->p.posY;
            fastNormVec(&state->vel.x, &state->vel.y);

            // Bump for 250ms
            state->state = TQ_EN_GUARD_ST_BUMPED;
            state->stateTimer = 250000;
        }
        // fall through
        case OBJ_BULLET_ARROW:
        case OBJ_BULLET_BOMB:
        case OBJ_BULLET_BOOMERANG:
        case OBJ_BULLET_SHIELD_0:
        case OBJ_BULLET_SHIELD_1:
        case OBJ_BULLET_SHIELD_2:
        case OBJ_BULLET_SHIELD_3:
        {
            enemy->health--;
            enemy->iFrameTimer = ENEMY_DEFAULT_IFRAMES_US;
            break;
        }
        default:
        {
            return;
        }
    }
}
