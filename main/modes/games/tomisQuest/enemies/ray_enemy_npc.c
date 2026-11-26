//==============================================================================
// Includes
//==============================================================================

#include "ray_enemy.h"
#include "ray_enemy_npc.h"
#include "ray_cutscene.h"

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsg_t* sprites[2];
    int32_t animTimer;
    int8_t spriteIdx;
} npcState_t;

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
 * @return true if the NPC should spawn, false otherwise
 */
bool rayInitEnemyNpc(ray_t* ray, rayEnemy_t* e, rayMapCellType_t type)
{
    // These fairies don't spawn in Tomi's House after the cutscene
    switch (type)
    {
        case OBJ_ENEMY_NPC_FAIRY_KAITIE:
        case OBJ_ENEMY_NPC_FAIRY_SABLE:
        case OBJ_ENEMY_NPC_FAIRY_BEAN:
        case OBJ_ENEMY_NPC_FAIRY_DACC:
        case OBJ_ENEMY_NPC_FAIRY_KIKI:
        case OBJ_ENEMY_NPC_FAIRY_BOUBA:
        {
            // If you're in map 0 (Tomi's house) and TQS_PAIRING was viewed
            if ((0 == ray->p.mapId) && (ray->p.cutsceneViewed & TQS_PAIRING))
            {
                // Don't spawn these fairies
                return false;
            }
        }
        default:
        {
            break;
        }
    }

    npcState_t* ns = heap_caps_calloc(1, sizeof(npcState_t), MALLOC_CAP_8BIT);

    // NPC has one texture, for now
    switch (type)
    {
        case OBJ_ENEMY_NPC_FAIRY_KAITIE:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_KAITIE_WSG, OBJ_ENEMY_NPC_FAIRY_KAITIE);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_KAITIE_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_FAIRY_SABLE:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_SABLE_WSG, OBJ_ENEMY_NPC_FAIRY_SABLE);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_SABLE_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_FAIRY_BEAN:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_BEAN_WSG, OBJ_ENEMY_NPC_FAIRY_BEAN);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_BEAN_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_FAIRY_DACC:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_DACC_WSG, OBJ_ENEMY_NPC_FAIRY_DACC);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_DACC_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_FAIRY_KIKI:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_KIKI_WSG, OBJ_ENEMY_NPC_FAIRY_KIKI);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_KIKI_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_FAIRY_BOUBA:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_BOUBA_WSG, OBJ_ENEMY_NPC_FAIRY_BOUBA);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_BOUBA_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_FAIRY_BARRY:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_BARRY_WSG, OBJ_ENEMY_NPC_FAIRY_BARRY);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_FAIRY_BARRY_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_HYUT:
        {
            ns->sprites[0] = loadTexture(ray, HYUT_D_0_WSG, OBJ_ENEMY_NPC_HYUT);
            ns->sprites[1] = loadTexture(ray, HYUT_D_2_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_GREAT_DEEJAY_TREE:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_GREAT_DEEJAY_TREE_WSG, OBJ_ENEMY_NPC_GREAT_DEEJAY_TREE);
            ns->sprites[1] = NULL;
            break;
        }
        case OBJ_ENEMY_NPC_LOOT:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_LOOT_WSG, OBJ_ENEMY_NPC_LOOT);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_LOOT_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_DORIA:
        {
            // In the field
            if ((5 == ray->p.mapId && (ray->p.cutsceneViewed & TQS_DORIA_INTRO)) ||
                // Dungeon 1
                (6 == ray->p.mapId && (ray->p.cutsceneViewed & TQS_DUNGEON_1_COMPLETE)) ||
                // Dungeon 2
                (7 == ray->p.mapId && (ray->p.cutsceneViewed & TQS_DUNGEON_2_COMPLETE)) ||
                // Dungeon 3
                (8 == ray->p.mapId && (ray->p.cutsceneViewed & TQS_DUNGEON_3_COMPLETE)) ||
                // Dungeon 4
                (9 == ray->p.mapId && (ray->p.cutsceneViewed & TQS_DUNGEON_4_COMPLETE)))
            {
                heap_caps_free(ns);
                return false;
            }
            else
            {
                // Good to spawn
                ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_DORIA_WSG, OBJ_ENEMY_NPC_DORIA);
                ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_DORIA_1_WSG, EMPTY);
            }
            break;
        }
        case OBJ_ENEMY_NPC_KING_DONUT:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_KING_DONUT_WSG, OBJ_ENEMY_NPC_KING_DONUT);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_KING_DONUT_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_MEGA_PULSE:
        {
            // After visiting Loot, before getting shield
            if (!ray->p.i.haveShield)
            {
                ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_MEGA_PULSE_WSG, OBJ_ENEMY_NPC_MEGA_PULSE);
                ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_MEGA_PULSE_1_WSG, EMPTY);
            }
            // After getting shield, before getting turntables
            else if (!ray->p.i.haveTurntables)
            {
                ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_SAWTOOTH_WSG, EMPTY);
                ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_SAWTOOTH_1_WSG, EMPTY);
            }
            // After getting turntables, before getting lullaby
            else if (!ray->p.i.haveDoriasLullaby)
            {
                ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_MEGA_PULSE_WSG, OBJ_ENEMY_NPC_MEGA_PULSE);
                ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_MEGA_PULSE_1_WSG, EMPTY);
            }
            // After getting lullaby
            else
            {
                ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_MEGA_PULSE_SAWTOOTH_WSG, EMPTY);
                ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_MEGA_PULSE_SAWTOOTH_1_WSG, EMPTY);
            }
            break;
        }
        case OBJ_ENEMY_NPC_CHO:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_CHO_WSG, OBJ_ENEMY_NPC_CHO);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_CHO_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_TRASH_MAN:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_TRASH_MAN_WSG, OBJ_ENEMY_NPC_TRASH_MAN);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_TRASH_MAN_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_EMBER_DEMON:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_EMBER_DEMON_WSG, OBJ_ENEMY_NPC_EMBER_DEMON);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_EMBER_DEMON_1_WSG, EMPTY);
            break;
        }
        case OBJ_ENEMY_NPC_THE_MAYOR:
        {
            // Mayor is actually a sign between getting the shield and turntables
            if (ray->p.i.haveShield && !ray->p.i.haveTurntables)
            {
                ns->sprites[0] = loadTexture(ray, OBJ_SCENERY_SIGNPOST_WSG, OBJ_SCENERY_SIGNPOST);
                ns->sprites[1] = NULL;
            }
            // After getting turntables, before getting lullaby
            else
            {
                ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_THE_MAYOR_WSG, OBJ_ENEMY_NPC_THE_MAYOR);
                ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_THE_MAYOR_1_WSG, EMPTY);
            }
            break;
        }
        case OBJ_ENEMY_NPC_FREDWARD:
        {
            ns->sprites[0] = loadTexture(ray, OBJ_ENEMY_NPC_FREDWARD_WSG, OBJ_ENEMY_NPC_FREDWARD);
            ns->sprites[1] = loadTexture(ray, OBJ_ENEMY_NPC_FREDWARD_1_WSG, EMPTY);
            break;
        }
        default:
        {
            // Not a valid NPC
            heap_caps_free(ns);
            return false;
        }
    }
    e->c.sprite = ns->sprites[0];
    e->portrait = e->c.sprite;

    // NPC has 1hp, indestructible
    e->health = 1;

    // Internal state for animation
    e->state = ns;
    // Offset everyone's animation to not be weirdly in sync
    ns->animTimer = (100000 * type) % 500000;

    // Set function pointers
    e->mainFn           = rayEnemyNpcMain;
    e->collisionFn      = rayEnemyNpcCheckPlayerCollision;
    e->getShotFn        = rayEnemyNpcGetShot;
    e->dropAfterDeathFn = NULL;

    return true;
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
    // Animate NPCs
    npcState_t* ns = enemy->state;
    if (ns && ns->sprites[1])
    {
        RUN_TIMER_EVERY(ns->animTimer, 500000, elapsedUs, {
            ns->spriteIdx   = (ns->spriteIdx + 1) % ARRAY_SIZE(ns->sprites);
            enemy->c.sprite = ns->sprites[ns->spriteIdx];
        });
    }

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
 * @brief Function called when an enemy is hit
 *
 * @param ray The whole game state
 * @param enemy The enemy which was hit
 * @param bullet The thing that hit the enemy
 */
static void rayEnemyNpcGetShot(ray_t* ray, rayEnemy_t* enemy, rayMapCellType_t bullet)
{
    switch (bullet)
    {
        case OBJ_BULLET_SWORD:
        case OBJ_BULLET_GHOST_SWORD:
        {
            // Show cutscene
            rayShowCutscene(ray, enemy->c.type, NULL);
            break;
        }
        default:
        {
            // Not damaged otherwise
            break;
        }
    }
}
