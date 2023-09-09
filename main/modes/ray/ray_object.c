//==============================================================================
// Includes
//==============================================================================

#include "ray_object.h"
#include "ray_tex_manager.h"
#include "ray_renderer.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool objectsIntersect(const rayObjCommon_t* obj1, const rayObjCommon_t* obj2);
static void moveRayBullets(ray_t* ray, int32_t elapsedUs);
static void moveRayEnemies(ray_t* ray, int32_t elapsedUs);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize templates for enemy creation
 *
 * @param ray The entire game state
 */
void initEnemyTemplates(ray_t* ray)
{
    const char* eTypes[] = {
        "NORMAL", "STRONG", "ARMORED", "FLAMING", "HIDDEN", "BOSS",
    };

    const rayMapCellType_t types[] = {
        OBJ_ENEMY_NORMAL, OBJ_ENEMY_STRONG, OBJ_ENEMY_ARMORED, OBJ_ENEMY_FLAMING, OBJ_ENEMY_HIDDEN, OBJ_ENEMY_BOSS,
    };

    char buf[64] = {0};
    for (int32_t eIdx = 0; eIdx < ARRAY_SIZE(ray->eTemplates); eIdx++)
    {
        // Set the type
        ray->eTemplates[eIdx].c.type = types[eIdx];

        // Load textures for all states
        for (int32_t frIdx = 0; frIdx < ARRAY_SIZE(ray->eTemplates->hurtSprites); frIdx++)
        {
            // Load the textures
            snprintf(buf, sizeof(buf) - 1, "E_%s_WALK_%" PRId32 ".wsg", eTypes[eIdx], frIdx);
            ray->eTemplates[eIdx].walkSprites[frIdx] = loadTexture(ray, buf, EMPTY);
            snprintf(buf, sizeof(buf) - 1, "E_%s_SHOOT_%" PRId32 ".wsg", eTypes[eIdx], frIdx);
            ray->eTemplates[eIdx].shootSprites[frIdx] = loadTexture(ray, buf, EMPTY);
            snprintf(buf, sizeof(buf) - 1, "E_%s_HURT_%" PRId32 ".wsg", eTypes[eIdx], frIdx);
            ray->eTemplates[eIdx].hurtSprites[frIdx] = loadTexture(ray, buf, EMPTY);
        }
        // Set initial texture
        ray->eTemplates[eIdx].c.sprite = getTexById(ray, ray->eTemplates[eIdx].walkSprites[0]);
    }
}

/**
 * @brief Initialize a bullet fired from the player's position in the player's direction based on the player's loadout
 *
 * @param ray The entire game state
 */
void fireShot(ray_t* ray)
{
    // Iterate over the bullet list, finding a new slot
    for (uint32_t newIdx = 0; newIdx < MAX_RAY_BULLETS; newIdx++)
    {
        // If this slot has a negative ID, use it
        if (-1 == ray->bullets[newIdx].c.id)
        {
            // Get a convenience pointer
            rayBullet_t* newBullet = &ray->bullets[newIdx];

            // Easy map for loadout to bullet type
            const rayMapCellType_t bulletMap[NUM_LOADOUTS] = {
                OBJ_BULLET_NORMAL, ///< Normal loadout
                // TODO charge beam unaccounted for
                OBJ_BULLET_MISSILE, ///< Missile loadout
                OBJ_BULLET_ICE,     ///< Ice beam loadout
                OBJ_BULLET_XRAY     ///< X-Ray loadout
            };
            // Get the bullet type for this loadout
            rayMapCellType_t bulletType = bulletMap[ray->loadout];

            // Initialize the bullet
            newBullet->c.type   = bulletType;
            wsg_t* texture      = getTexByType(ray, bulletType);
            newBullet->c.sprite = texture;
            // Spawn it slightly in front of the player
            newBullet->c.posX = ray->posX + ray->dirX / 2;
            newBullet->c.posY = ray->posY + ray->dirY / 2;
            // Velocity is the player's direction
            newBullet->velX = ray->dirX;
            newBullet->velY = ray->dirY;

            // Width is based on the texture width as a fraction of a cell
            newBullet->c.radius = TO_FX_FRAC(texture->w, TEX_WIDTH);
            // Bullets don't have real IDs
            newBullet->c.id = 0;

            // All done
            return;
        }
    }
}

/**
 * @brief Move all bullets and enemies
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void moveRayObjects(ray_t* ray, int32_t elapsedUs)
{
    moveRayBullets(ray, elapsedUs);
    moveRayEnemies(ray, elapsedUs);
}

/**
 * @brief Move all bullets and check for collisions with doors
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
static void moveRayBullets(ray_t* ray, int32_t elapsedUs)
{
    // For each bullet slot
    for (uint32_t i = 0; i < MAX_RAY_BULLETS; i++)
    {
        // If a bullet is in the slot
        rayBullet_t* obj = &(ray->bullets[i]);
        if (0 == obj->c.id)
        {
            // Update the bullet's position
            // TODO justify the scaling factor
            obj->c.posX += (obj->velX * elapsedUs) / 100000;
            obj->c.posY += (obj->velY * elapsedUs) / 100000;

            // Get the cell the bullet is in now
            rayMapCell_t* cell = &ray->map.tiles[FROM_FX(obj->c.posX)][FROM_FX(obj->c.posY)];

            // If a wall is it
            if (CELL_IS_TYPE(cell->type, BG | WALL))
            {
                // Destroy this bullet
                memset(obj, 0, sizeof(rayBullet_t));
                obj->c.id = -1;
            }
            // Else if a door is hit
            else if (CELL_IS_TYPE(cell->type, BG | DOOR))
            {
                // Check if the bullet type can open the door
                // TODO handle charge beam
                if ((BG_DOOR == cell->type) // Normal doors are openable by anything
                    || (BG_DOOR_CHARGE == cell->type /*&& OBJ_BULLET_CHARGE == obj->type*/)
                    || (BG_DOOR_MISSILE == cell->type && OBJ_BULLET_MISSILE == obj->c.type)
                    || (BG_DOOR_ICE == cell->type && OBJ_BULLET_ICE == obj->c.type)
                    || (BG_DOOR_XRAY == cell->type && OBJ_BULLET_XRAY == obj->c.type))
                {
                    // If the door is closed
                    if (0 == cell->doorOpen)
                    {
                        // Start opening the door
                        cell->doorOpen = 1;
                        // Destroy this bullet
                        memset(obj, 0, sizeof(rayBullet_t));
                        obj->c.id = -1;
                    }
                }
            }
        }
    }
}

/**
 * @brief Move all enemies
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
static void moveRayEnemies(ray_t* ray, int32_t elapsedUs)
{
    // Iterate over the linked list
    node_t* currentNode = ray->enemies.first;
    while (currentNode != NULL)
    {
        // Get a pointer from the linked list
        rayEnemy_t* obj = ((rayEnemy_t*)currentNode->val);

        // TODO move enemies

        // Iterate to the next node
        currentNode = currentNode->next;
    }
}

/**
 * @brief Check if two ::rayObjCommon_t intersect
 *
 * @param obj1 The first rayObjCommon_t to check for intersection
 * @param obj2 The second rayObjCommon_t to check for intersection
 * @return true if they intersect, false if they do not
 */
static bool objectsIntersect(const rayObjCommon_t* obj1, const rayObjCommon_t* obj2)
{
    q24_8 deltaX    = (obj2->posX - obj1->posX);
    q24_8 deltaY    = (obj2->posY - obj1->posY);
    q24_8 radiusSum = (obj1->radius + obj2->radius);
    return (deltaX * deltaX) + (deltaY * deltaY) < (radiusSum * radiusSum);
}

/**
 * @brief Check for collisions between bullets, enemies, and the player
 *
 * @param ray The entire game state
 */
void checkRayCollisions(ray_t* ray)
{
    // TODO collision checks
}
