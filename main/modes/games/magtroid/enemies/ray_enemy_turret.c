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
 * @brief Main loop for the Turret enemy. Handles attacking logic.
 *
 * @param ray The whole game state
 * @param enemy the Turret enemy
 * @param elapsedUs The time since this function was last called
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

    // Shoot a random bullet towards the player every two seconds
    RUN_TIMER_EVERY(state->dirTimer, 2000000, elapsedUs, {
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
    });

    // Not dead yet!
    return false;
}

/**
 * @brief Function to check if the turret collides with the player
 * The turret is immoveable, so collisions stop player movement
 *
 * @param ray The whole game state
 * @param enemy The turret to check for collisions
 * @param player The player to check for collisions
 * @param deltaX The X distance the player is trying to move
 * @param deltaY The Y distance the player is trying to move
 */
void rayEnemyTurretCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
{
    // Check if there's a bounding box intersection
    rectangle_t enemyBB = rayGetObjBB(&enemy->c);
    if (rectRectIntersection(player, enemyBB, NULL))
    {
        *deltaX = 0;
        *deltaY = 0;
    }
}

/**
 * @brief This function is called when the turret is hit
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
            // Reflected bullets damage the turret
            // TODO start enemy iframes
            // TODO visual indicator enemy was hit
            enemy->health--;
            break;
        }
        case OBJ_BULLET_SWORD:
        case OBJ_BULLET_ARROW:
        case OBJ_BULLET_BOMB:
        case OBJ_BULLET_BOOMERANG:
        default:
        {
            // No damage from other weapons
            break;
        }
    }
}
