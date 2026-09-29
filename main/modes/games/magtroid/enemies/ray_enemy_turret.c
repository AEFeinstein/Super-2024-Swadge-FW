//==============================================================================
// Includes
//==============================================================================

#include "ray_enemy.h"
#include "ray_enemy_turret.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    int32_t dirTimer; ///< A timer to randomly pick new directions to face
} turretState_t;

//==============================================================================
// Function Declarations
//==============================================================================

static bool rayEnemyTurretMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
static void rayEnemyTurretCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX,
                                               q24_8* deltaY);
static void rayEnemyTurretGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a turret
 *
 * @param ray The whole game state
 * @param e The turret to initialize
 */
void rayInitEnemyTurret(ray_t* ray, rayEnemy_t* e)
{
    // Turret has one texture, for now
    e->c.sprite = loadTexture(ray, OBJ_ENEMY_TURRET_WSG, OBJ_ENEMY_TURRET);

    // Also load bullet textures
    loadTexture(ray, OBJ_BULLET_SHIELD_0_WSG, OBJ_BULLET_SHIELD_0);
    loadTexture(ray, OBJ_BULLET_SHIELD_1_WSG, OBJ_BULLET_SHIELD_1);
    loadTexture(ray, OBJ_BULLET_SHIELD_2_WSG, OBJ_BULLET_SHIELD_2);
    loadTexture(ray, OBJ_BULLET_SHIELD_3_WSG, OBJ_BULLET_SHIELD_3);

    // Turret has 5hp
    e->health = 5;

    // Allocate state for this enemy
    e->state = heap_caps_calloc(1, sizeof(turretState_t), MALLOC_CAP_8BIT);

    // Set function pointers
    e->mainFn           = rayEnemyTurretMain;
    e->collisionFn      = rayEnemyTurretCheckPlayerCollision;
    e->getShotFn        = rayEnemyTurretGetShot;
    e->dropAfterDeathFn = rayEnemyStandardItemDrop;
}

/**
 * @brief TODO doc
 *
 * @param ray The whole game state
 * @param enemy
 * @param elapsedUs
 * @return true if this enemy is dead (after any death animations), false if it is alive
 */
bool rayEnemyTurretMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Return if dead for cleanup
    if (enemy->health <= 0)
    {
        return true;
    }

    // Casted pointer for convenience
    turretState_t* state = enemy->state;

    // Shoot direction every second
    RUN_TIMER_EVERY(state->dirTimer, 2000000, elapsedUs, {
        vec_q24_8 dirToPlayer;
        dirToPlayer.x = ray->p.posX - enemy->c.posX;
        dirToPlayer.y = ray->p.posY - enemy->c.posY;
        fastNormVec(&dirToPlayer.x, &dirToPlayer.y);

        // Scale to 3/8 speed
        dirToPlayer.x = (3 * dirToPlayer.x) / 8;
        dirToPlayer.y = (3 * dirToPlayer.y) / 8;

        rayCreateBullet(ray,                                      //
                        OBJ_BULLET_SHIELD_0 + (esp_random() % 4), // Type
                        enemy->c.posX, enemy->c.posY,             // Position
                        dirToPlayer.x, dirToPlayer.y,             // Velocity
                        0, 0,                                     // Acceleration
                        -1, false);
    });

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
void rayEnemyTurretCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
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
                // Give the player a bump away from the turret
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
void rayEnemyTurretGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    switch (bullet)
    {
        case OBJ_BULLET_SHIELD_0:
        case OBJ_BULLET_SHIELD_1:
        case OBJ_BULLET_SHIELD_2:
        case OBJ_BULLET_SHIELD_3:
        {
            // TODO start enemy iframes
            // TODO visual indicator enemy was hit
            enemy->health--;
            break;
        }
        default:
        {
            // No damage from other weapons
            break;
        }
    }
}
