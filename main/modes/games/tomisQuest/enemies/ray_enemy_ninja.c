//==============================================================================
// Includes
//==============================================================================

#include "ray_enemy.h"
#include "ray_enemy_ninja.h"

//==============================================================================
// Enums
//==============================================================================

typedef enum
{
    TQ_EN_NINJA_ST_HIDING,
    TQ_EN_NINJA_ST_ATTACKING,
    TQ_EN_NINJA_ST_WANDERING,
    TQ_EN_NINJA_ST_BUMPED
} ninjaStateIndex_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    ninjaStateIndex_t state;
    int32_t stateTimer;
    vec_q24_8 dir;     ///< The direction the ninja is facing
    vec_q24_8 vel; ///< The direction the ninja got bumped
} ninjaState_t;

//==============================================================================
// Function Declarations
//==============================================================================

static bool rayEnemyNinjaMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
static void rayEnemyNinjaCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX,
                                              q24_8* deltaY);
static void rayEnemyNinjaGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);
static void rayEnemyNinjaPickDirection(rayEnemy_t* enemy);
static void rayEnemyNinjaShoot(ray_t* ray, rayEnemy_t* enemy);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a ninja
 *
 * @param ray The whole game state
 * @param e The ninja to initialize
 */
void rayInitEnemyNinja(ray_t* ray, rayEnemy_t* e)
{
    // Ninja has one texture, for now
    e->c.sprite = loadTexture(ray, BG_WALL_BOX_WSG, OBJ_ENEMY_NINJA);
    e->portrait = e->c.sprite;

    // Load bullet textures
    loadTexture(ray, OBJ_BULLET_SHIELD_0_WSG, OBJ_BULLET_SHIELD_0);
    loadTexture(ray, OBJ_BULLET_SHIELD_1_WSG, OBJ_BULLET_SHIELD_1);
    loadTexture(ray, OBJ_BULLET_SHIELD_2_WSG, OBJ_BULLET_SHIELD_2);
    loadTexture(ray, OBJ_BULLET_SHIELD_3_WSG, OBJ_BULLET_SHIELD_3);

    // Ninja has 2hp to not die
    e->health = 2;

    // Allocate state for this enemy
    e->state = heap_caps_calloc(1, sizeof(ninjaState_t), MALLOC_CAP_8BIT);

    ((ninjaState_t*) e->state)->state = TQ_EN_NINJA_ST_HIDING;
    ((ninjaState_t*) e->state)->stateTimer = -1;

    // Set function pointers
    e->mainFn           = rayEnemyNinjaMain;
    e->collisionFn      = rayEnemyNinjaCheckPlayerCollision;
    e->getShotFn        = rayEnemyNinjaGetShot;
    e->dropAfterDeathFn = rayEnemyStandardItemDrop;

    // Pick a random direction to start watching
    rayEnemyNinjaPickDirection(e);
}

/**
 * @brief Pick a new direction for the ninja to travel, adjusting direction and sprite rotation
 *
 * @param enemy The ninja which is picking a new direction to travel
 */
static void rayEnemyNinjaPickDirection(rayEnemy_t* enemy)
{
    ninjaState_t* state = enemy->state;

    // Pick a random cardinal direction
    state->dir.x            = 0;
    state->dir.y            = 0;
    enemy->c.spriteRotation = 0;
    switch (esp_random() & 0x03)
    {
        case 0x00:
        {
            state->dir.x            = TO_FX(1);
            enemy->c.spriteRotation = 90;
            break;
        }
        case 0x01:
        {
            state->dir.x            = -TO_FX(1);
            enemy->c.spriteRotation = 270;
            break;
        }
        case 0x02:
        {
            state->dir.y            = TO_FX(1);
            enemy->c.spriteRotation = 180;
            break;
        }
        case 0x03:
        {
            state->dir.y            = -TO_FX(1);
            enemy->c.spriteRotation = 0;
            break;
        }
    }
}

void rayEnemyNinjaShoot(ray_t* ray, rayEnemy_t* enemy)
{
    // Find the direction from the turret to the player
        vec_q24_8 dirToPlayer;
        dirToPlayer.x = ray->p.posX - enemy->c.posX;
        dirToPlayer.y = ray->p.posY - enemy->c.posY;
        fastNormVec(&dirToPlayer.x, &dirToPlayer.y);

        // Scale to 3/8 speed
        dirToPlayer.x = (3 * dirToPlayer.x) / 8;
        dirToPlayer.y = (3 * dirToPlayer.y) / 8;

        // Shoot a bullet
        rayCreateBullet(ray,                                      //
                        OBJ_BULLET_SHIELD_0 + (esp_random() % 4), // Type
                        enemy->c.posX, enemy->c.posY,             // Position
                        dirToPlayer.x, dirToPlayer.y,             // Velocity
                        0, 0,                                     // Acceleration
                        -1, false);
}

/**
 * @brief Main loop for the Ninja enemy. Handles movement logic.
 *
 * @param ray The whole game state
 * @param enemy The Ninja enemy
 * @param elapsedUs The time since this function was last called
 * @return true if this enemy is dead (after any death animations), false if it is alive
 */
bool rayEnemyNinjaMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Return if dead for cleanup
    if (enemy->health <= 0)
    {
        return true;
    }

    // Casted pointer for convenience
    ninjaState_t* state = enemy->state;

    switch(state->state)
    {
        case TQ_EN_NINJA_ST_HIDING:
        default:
        {
            enemy->c.spriteRotation = 0;
            state->vel.x = 0;
            state->vel.y = 0;

            rectangle_t proximityBox = {
                .pos = {.x = enemy->c.posX - 512, .y = enemy->c.posY - 512},
                .width = 1024,
                .height = 1024
            };

            if(rectRectIntersection(rayGetPlayerBB(ray), proximityBox, NULL))
            {
                enemy->c.sprite = loadTexture(ray, OBJ_ENEMY_GRUNT_WSG, OBJ_ENEMY_NINJA);
                state->state = TQ_EN_NINJA_ST_ATTACKING;
                rayEnemyNinjaPickDirection(enemy);
                state->stateTimer = 500000;
            }
            
            break;
        }
        case TQ_EN_NINJA_ST_ATTACKING:
        {
            state->vel = fpvMulSc(state->dir, TO_FX_FRAC(4,3));

            state->stateTimer-=elapsedUs;
            if(state->stateTimer < 0)
            {
                rayEnemyNinjaShoot(ray, enemy);
                state->state = TQ_EN_NINJA_ST_WANDERING;
                state->stateTimer = 2000000;
            }

            break;
        }
        case TQ_EN_NINJA_ST_WANDERING:
        {
            state->vel = state->dir;
                
            state->stateTimer-=elapsedUs;
            if(state->stateTimer < 0)
            {
                state->state = TQ_EN_NINJA_ST_HIDING;
                enemy->c.sprite = loadTexture(ray, BG_WALL_BOX_WSG, OBJ_ENEMY_NINJA);
            }

            break;
        }
        case TQ_EN_NINJA_ST_BUMPED:
        {
            state->stateTimer -= elapsedUs;

            if(state->stateTimer <= 0)
            {
                state->state = TQ_EN_NINJA_ST_ATTACKING;
                rayEnemyNinjaPickDirection(enemy);
                state->stateTimer = 2000000;
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
        rayEnemyNinjaPickDirection(enemy);
        state->stateTimer += 10000;
    }

    // Not dead yet!
    return false;
}

/**
 * @brief Function to check if the ninja collides with the player
 * Touching the ninja damages and bumps the player
 *
 * @param ray The whole game state
 * @param enemy The ninja to check for collisions
 * @param player The player to check for collisions
 * @param deltaX The X distance the player is trying to move
 * @param deltaY The Y distance the player is trying to move
 */
void rayEnemyNinjaCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
{
    ninjaState_t* state = enemy->state;

    if(state->state == TQ_EN_NINJA_ST_HIDING)
    {
        return;
    }
    
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
                // Give the player a bump away from the ninja
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
 * @brief This function is called when the ninja is hit
 *
 * @param ray The whole game state
 * @param enemy The enemy which was shot
 * @param bullet OBJ_BULLET_ARROW, OBJ_BULLET_BOMB, OBJ_BULLET_BOOMERANG, or OBJ_BULLET_SWORD
 */
void rayEnemyNinjaGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    // Convenience pointer
    ninjaState_t* state = enemy->state;

    switch (bullet)
    {
        case OBJ_BULLET_SWORD:
        {
            if(state->state == TQ_EN_NINJA_ST_HIDING)
            {
                return;
            }

            // Give the player a bump away from the ninja
            state->vel.x = enemy->c.posX - ray->p.posX;
            state->vel.y = enemy->c.posY - ray->p.posY;
            fastNormVec(&state->vel.x, &state->vel.y);
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

            state->state = TQ_EN_NINJA_ST_BUMPED;
            state->stateTimer = 250000;
            break;
        }
        default:
        {
            return;
        }
    }
}
