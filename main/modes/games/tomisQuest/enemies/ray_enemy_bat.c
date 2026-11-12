//==============================================================================
// Includes
//==============================================================================

#include "ray_enemy.h"
#include "ray_enemy_bat.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    TQ_EN_BAT_ST_IDLE,
    TQ_EN_BAT_ST_FLYING,
    TQ_EN_BAT_ST_BUMPED
} batStateIndex_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    batStateIndex_t state;
    int32_t stateTimer; // In microseconds
    int8_t stateCounter;
    vec_q24_8 vel; ///< The direction the bat got bumped
} batState_t;

//==============================================================================
// Function Declarations
//==============================================================================

static bool rayEnemyBatMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
static void rayEnemyBatCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX,
                                            q24_8* deltaY);
static void rayEnemyBatGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a bat
 *
 * @param ray The whole game state
 * @param e The bat to initialize
 */
void rayInitEnemyBat(ray_t* ray, rayEnemy_t* e)
{
    // Bat has one texture, for now
    e->c.sprite = loadTexture(ray, OBJ_ENEMY_GRUNT_WSG, OBJ_ENEMY_BAT);
    e->portrait = e->c.sprite;

    // Bat has 1hp to not die
    e->health = 1;

    // Allocate state for this enemy
    e->state = heap_caps_calloc(1, sizeof(batState_t), MALLOC_CAP_8BIT);

    ((batState_t*)e->state)->state      = TQ_EN_BAT_ST_IDLE;
    ((batState_t*)e->state)->stateTimer = -1;

    // Set function pointers
    e->mainFn           = rayEnemyBatMain;
    e->collisionFn      = rayEnemyBatCheckPlayerCollision;
    e->getShotFn        = rayEnemyBatGetShot;
    e->dropAfterDeathFn = rayEnemyStandardItemDrop;
}

/**
 * @brief Main loop for the Bat enemy. Handles movement logic.
 *
 * @param ray The whole game state
 * @param enemy The Bat enemy
 * @param elapsedUs The time since this function was last called
 * @return true if this enemy is dead (after any death animations), false if it is alive
 */
bool rayEnemyBatMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Return if dead for cleanup
    if (enemy->health <= 0)
    {
        return true;
    }

    // Casted pointer for convenience
    batState_t* state = enemy->state;

    switch (state->state)
    {
        case TQ_EN_BAT_ST_IDLE:
        default:
        {
            // enemy->c.spriteRotation = 0;
            state->vel.x = 0;
            state->vel.y = 0;

            rectangle_t proximityBox = {.pos    = {.x = enemy->c.posX - TO_FX(2), .y = enemy->c.posY - TO_FX(2)},
                                        .width  = TO_FX(4),
                                        .height = TO_FX(4)};

            if (rectRectIntersection(rayGetPlayerBB(ray), proximityBox, NULL))
            {
                enemy->c.sprite = loadTexture(ray, OBJ_ENEMY_GRUNT_WSG, OBJ_ENEMY_BAT);
                state->state    = TQ_EN_BAT_ST_FLYING;

                state->vel.x = (ray->p.posX - enemy->c.posX) < 0 ? 150 : -150;
                state->vel.y = (ray->p.posY - enemy->c.posY) < 0 ? 150 : -150;

                state->stateTimer   = 200000;
                state->stateCounter = 29;
            }

            break;
        }
        case TQ_EN_BAT_ST_FLYING:
        {
            state->stateTimer -= elapsedUs;
            if (state->stateTimer < 0)
            {
                if (!(state->stateCounter % 10))
                {
                    state->vel.x = (ray->p.posX - enemy->c.posX) < 0 ? -150 : 150;
                    state->vel.y = (ray->p.posY - enemy->c.posY) < 0 ? -150 : 150;
                }
                else
                {
                    vec_t v      = {.x = state->vel.x, .y = state->vel.y};
                    vec_t rv     = rotateVec2d(v, -40 + (esp_random() % 80));
                    vec_q24_8 nv = {.x = rv.x, .y = rv.y};
                    state->vel   = nv;

                    state->stateTimer = 200000;
                }

                state->stateCounter--;
                if (state->stateCounter < 0)
                {
                    state->state = TQ_EN_BAT_ST_IDLE;
                }
            }

            break;
        }
        case TQ_EN_BAT_ST_BUMPED:
        {
            state->stateTimer -= elapsedUs;

            if (state->stateTimer <= 0)
            {
                state->state        = TQ_EN_BAT_ST_FLYING;
                state->stateCounter = 29;
                state->stateTimer   = 200000;
            }
            break;
        }
    }

    // Move half as fast as than the player
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
        state->vel.x = -state->vel.x;
        state->vel.y = -state->vel.y;
    }

    // Not dead yet!
    return false;
}

/**
 * @brief Function to check if the bat collides with the player
 * Touching the bat damages and bumps the player
 *
 * @param ray The whole game state
 * @param enemy The bat to check for collisions
 * @param player The player to check for collisions
 * @param deltaX The X distance the player is trying to move
 * @param deltaY The Y distance the player is trying to move
 */
void rayEnemyBatCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
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
                // Give the player a bump away from the bat
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
 * @brief This function is called when the bat is hit
 *
 * @param ray The whole game state
 * @param enemy The enemy which was shot
 * @param bullet OBJ_BULLET_ARROW, OBJ_BULLET_BOMB, OBJ_BULLET_BOOMERANG, or OBJ_BULLET_SWORD
 */
void rayEnemyBatGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // Convenience pointer
    batState_t* state = enemy->state;

    switch (bullet)
    {
        case OBJ_BULLET_SWORD:
        {
            // Give the player a bump away from the bat
            state->vel.x = enemy->c.posX - ray->p.posX;
            state->vel.y = enemy->c.posY - ray->p.posY;
            fastNormVec(&state->vel.x, &state->vel.y);

            // Bump for 250ms
            state->state      = TQ_EN_BAT_ST_BUMPED;
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
