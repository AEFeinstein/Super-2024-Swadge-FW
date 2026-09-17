//==============================================================================
// Includes
//==============================================================================

#include "ray_enemy.h"
#include "ray_enemy_grunt.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    vec_q24_8 dir;     ///< The direction the grunt is facing
    int32_t dirTimer;  ///< A timer to randomly pick new directions to face
    vec_q24_8 bumpVel; ///< The direction the grunt got bumped
    int32_t bumpTimer; ///< A timer to use bumpVel instead of dir for movement
} gruntState_t;

//==============================================================================
// Function Declarations
//==============================================================================

static bool rayEnemyGruntMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
static void rayEnemyGruntCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX,
                                              q24_8* deltaY);
static void rayEnemyGruntGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
static void rayEnemyGruntPickDirection(rayEnemy_t* enemy);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a grunt
 *
 * @param ray The whole game state
 * @param e The grunt to initialize
 */
void rayInitEnemyGrunt(ray_t* ray, rayEnemy_t* e)
{
    // Grunt has one texture, for now
    e->c.sprite = loadTexture(ray, OBJ_ENEMY_GRUNT_WSG, OBJ_ENEMY_GRUNT);

    // Grunt has 1hp to not die
    e->health = 3;

    // Allocate state for this enemy
    e->state = heap_caps_calloc(1, sizeof(gruntState_t), MALLOC_CAP_8BIT);

    // Set function pointers
    e->mainFn           = rayEnemyGruntMain;
    e->collisionFn      = rayEnemyGruntCheckPlayerCollision;
    e->getShotFn        = rayEnemyGruntGetShot;
    e->dropAfterDeathFn = rayEnemyStandardItemDrop;

    // Pick a random direction to start traveling
    rayEnemyGruntPickDirection(e);
}

/**
 * @brief Pick a new direction for the grunt to travel
 *
 * @param enemy
 */
static void rayEnemyGruntPickDirection(rayEnemy_t* enemy)
{
    gruntState_t* state = enemy->state;

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
 * @brief TODO doc
 *
 * @param ray The whole game state
 * @param enemy
 * @param elapsedUs
 * @return true if this enemy is dead (after any death animations), false if it is alive
 */
bool rayEnemyGruntMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Return if dead for cleanup
    if (enemy->health <= 0)
    {
        enemyDropAfterDeath(enemy);
        return true;
    }

    // Casted pointer for convenience
    gruntState_t* state = enemy->state;

    // Change direction every second
    RUN_TIMER_EVERY(state->dirTimer, 1000000, elapsedUs, { rayEnemyGruntPickDirection(enemy); });

    // Pick the velocity to use based on bumpTimer
    vec_q24_8 dir;
    if (state->bumpTimer > 0)
    {
        state->bumpTimer -= elapsedUs;
        dir = state->bumpVel;
    }
    else
    {
        dir = state->dir;
    }

    // Move half as fast as than the player
    vec_q24_8 delta = {
        .x = (dir.x * (int32_t)elapsedUs) / (40000 * 6),
        .y = (dir.y * (int32_t)elapsedUs) / (40000 * 6),
    };

    // Get a bounding box for where the enemy is
    rectangle_t bb = rayGetObjBB(&enemy->c);

    // Move the bounding box without moving the enemy yet
    bb.pos.x += delta.x;
    bb.pos.y += delta.y;

    // Update the enemy position if it's in bounds
    if (rayBoundingBoxFitsInMap(ray, bb))
    {
        enemy->c.posX += delta.x;
        enemy->c.posY += delta.y;
    }
    else
    {
        // Can't move this way, pick a new direction
        rayEnemyGruntPickDirection(enemy);
        state->dirTimer = 0;
    }

    // Not dead yet!
    return false;
}

/**
 * @brief TODO doc
 *
 * @param ray The whole game state
 * @param enemy
 * @param player
 * @param deltaX
 * @param deltaY
 */
void rayEnemyGruntCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
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
                // Give the player a bump away from the grunt
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
 * @brief TODO doc
 *
 * @param ray The whole game state
 * @param enemy The enemy which was shot
 * @param bullet OBJ_BULLET_ARROW, OBJ_BULLET_BOMB, OBJ_BULLET_BOOMERANG, or OBJ_BULLET_SWORD
 */
void rayEnemyGruntGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // Convenience pointer
    gruntState_t* state = enemy->state;

    switch (bullet)
    {
        case OBJ_BULLET_SWORD:
        {
            // Give the player a bump away from the grunt
            state->bumpVel.x = enemy->c.posX - ray->p.posX;
            state->bumpVel.y = enemy->c.posY - ray->p.posY;
            fastNormVec(&state->bumpVel.x, &state->bumpVel.y);

            // Bump for 250ms
            state->bumpTimer = 250000;
        }
        // fall through
        case OBJ_BULLET_ARROW:
        case OBJ_BULLET_BOMB:
        case OBJ_BULLET_BOOMERANG:
        {
            // TODO start enemy iframes
            // TODO visual indicator enemy was hit
            enemy->health--;
            break;
        }
        default:
        {
            return;
        }
    }
}
