//==============================================================================
// Includes
//==============================================================================

#include "ray_enemy.h"
#include "ray_enemy_npc.h"
#include "ray_cutscene.h"

//==============================================================================
// Function Declarations
//==============================================================================

static bool rayEnemyNpcMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs);
static void rayEnemyNpcCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX,
                                            q24_8* deltaY);
static void rayEnemyNpcGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize a npc
 *
 * @param ray The whole game state
 * @param e The npc to initialize
 */
void rayInitEnemyNpc(ray_t* ray, rayEnemy_t* e, rayMapCellType_t type)
{
    // NPC has one texture, for now
    cnfsFileIdx_t tex;
    switch (type)
    {
        case OBJ_ENEMY_FAIRY_0 ... OBJ_ENEMY_FAIRY_5:
        {
            tex = OBJ_ENEMY_FAIRY_0_WSG + (type - OBJ_ENEMY_FAIRY_0);
            break;
        }
        default:
        {
            // Not a valid NPC
            return;
        }
    }
    e->c.sprite = loadTexture(ray, tex, type);
    e->portrait = e->c.sprite;

    // NPC has 1hp, indestructible
    e->health = 1;

    // No internal state
    e->state = NULL;

    // Set function pointers
    e->mainFn           = rayEnemyNpcMain;
    e->collisionFn      = rayEnemyNpcCheckPlayerCollision;
    e->getShotFn        = rayEnemyNpcGetShot;
    e->dropAfterDeathFn = NULL;
}

/**
 * @brief Main loop for the Npc enemy. Handles attacking logic.
 *
 * @param ray The whole game state
 * @param enemy the Npc enemy
 * @param elapsedUs The time since this function was last called
 * @return true if this enemy is dead (after any death animations), false if it is alive
 */
bool rayEnemyNpcMain(ray_t* ray, rayEnemy_t* enemy, uint32_t elapsedUs)
{
    // Invincible!
    return false;
}

/**
 * @brief Function to check if the npc collides with the player
 * The npc is immoveable, so collisions stop player movement
 *
 * @param ray The whole game state
 * @param enemy The npc to check for collisions
 * @param player The player to check for collisions
 * @param deltaX The X distance the player is trying to move
 * @param deltaY The Y distance the player is trying to move
 */
void rayEnemyNpcCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
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
 * @brief TODO doc
 *
 * @param ray
 * @param enemy
 * @param bullet
 */
static void rayEnemyNpcGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    switch (bullet)
    {
        case OBJ_BULLET_SWORD:
        case OBJ_BULLET_GHOST_SWORD:
        {
            // Show cutscene
            rayShowCutscene(ray, enemy->c.type, -1);
            break;
        }
        default:
        {
            break;
        }
    }
}
