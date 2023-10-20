//==============================================================================
// Includes
//==============================================================================

#include "ray_object.h"
#include "ray_tex_manager.h"
#include "ray_renderer.h"
#include "ray_map.h"
#include "ray_player.h"
#include "ray_dialog.h"
#include "ray_script.h"
#include "ray_enemy.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool objectsIntersect(const rayObjCommon_t* obj1, const rayObjCommon_t* obj2);
static void moveRayBullets(ray_t* ray, int32_t elapsedUs);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize templates for enemy creation. This also loads enemy sprites
 *
 * @param ray The entire game state
 */
void initEnemyTemplates(ray_t* ray)
{
    // The names of the different types of enemies
    const char* eTypes[] = {
        "NORMAL", "STRONG", "ARMORED", "FLAMING", "HIDDEN", "BOSS",
    };

    // The types of enemies
    const rayMapCellType_t types[] = {
        OBJ_ENEMY_NORMAL, OBJ_ENEMY_STRONG, OBJ_ENEMY_ARMORED, OBJ_ENEMY_FLAMING, OBJ_ENEMY_HIDDEN, OBJ_ENEMY_BOSS,
    };

    // An empty buffer to build strings
    char buf[64] = {0};

    // For each enemy type
    for (int32_t eIdx = 0; eIdx < ARRAY_SIZE(ray->eTemplates); eIdx++)
    {
        // Set the type
        ray->eTemplates[eIdx].c.type = types[eIdx];

        // Set the time for each animation state
        ray->eTemplates[eIdx].animTimerLimit = 250000;

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
        ray->eTemplates[eIdx].c.sprite = ray->eTemplates[eIdx].walkSprites[0];
    }
}

/**
 * @brief Create a bullet with an owner, type, position, and velocity
 *
 * @param ray The entire game state
 * @param bulletType The type of bullet
 * @param posX The X position of the spawner. Bullet will be positioned slightly in front of the given position
 * @param posY The X position of the spawner. Bullet will be positioned slightly in front of the given position
 * @param velX The X velocity of the bullet
 * @param velY The Y velocity of the bullet
 * @param isPlayer true if this is the player's shot, false if it is an enemy's shot
 */
void rayCreateBullet(ray_t* ray, rayMapCellType_t bulletType, q24_8 posX, q24_8 posY, q24_8 velX, q24_8 velY,
                     bool isPlayer)
{
    // Iterate over the bullet list, finding a new slot
    for (uint32_t newIdx = 0; newIdx < MAX_RAY_BULLETS; newIdx++)
    {
        // If this slot has a negative ID, use it
        if (-1 == ray->bullets[newIdx].c.id)
        {
            // Get a convenience pointer
            rayBullet_t* newBullet = &ray->bullets[newIdx];

            // Initialize the bullet
            newBullet->c.type = bulletType;
            // Bullets IDs are just if it's owned by the player or not
            newBullet->c.id = isPlayer ? 1 : 0;

            // Set the texture
            wsg_t* texture      = getTexByType(ray, bulletType);
            newBullet->c.sprite = texture;
            // Width is based on the texture width as a fraction of a cell
            newBullet->c.radius = TO_FX_FRAC(texture->w, 2 * TEX_WIDTH);

            // Spawn it slightly in front of the shooter's position
            newBullet->c.posX = posX + (velX / 4);
            newBullet->c.posY = posY + (velY / 4);

            // Set the velocity
            newBullet->velX = velX;
            newBullet->velY = velY;

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
    rayEnemiesMoveAnimate(ray, elapsedUs);
}

/**
 * @brief Move all bullets and check for collisions with doors
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
static void moveRayBullets(ray_t* ray, int32_t elapsedUs)
{
    // For convenience
    int32_t rayMapId = ray->p.mapId;

    // For each bullet slot
    for (uint32_t i = 0; i < MAX_RAY_BULLETS; i++)
    {
        // If a bullet is in the slot
        rayBullet_t* obj = &(ray->bullets[i]);
        if (-1 != obj->c.id)
        {
            // Update the bullet's position
            // TODO justify the scaling factor, assuming velXY is a unit vector
            obj->c.posX += (obj->velX * elapsedUs) / 100000;
            obj->c.posY += (obj->velY * elapsedUs) / 100000;

            // Get the cell the bullet is in now
            rayMapCell_t* cell = &ray->map.tiles[FROM_FX(obj->c.posX)][FROM_FX(obj->c.posY)];

            // If a wall is it
            if (CELL_IS_TYPE(cell->type, BG | WALL))
            {
                // Check for shot wall scripts
                if (checkScriptShootWall(ray, FROM_FX(obj->c.posX), FROM_FX(obj->c.posY)))
                {
                    // Script warped, return
                    return;
                }

                // Destroy this bullet
                memset(obj, 0, sizeof(rayBullet_t));
                obj->c.id = -1;
            }
            // Else if a door is hit
            else if (CELL_IS_TYPE(cell->type, BG | DOOR))
            {
                // If the door is closed
                if (0 == cell->doorOpen)
                {
                    // Check if the bullet type can open the door
                    if ((BG_DOOR == cell->type) // Normal doors are openable by anything
                        || (BG_DOOR_CHARGE == cell->type && OBJ_BULLET_CHARGE == obj->c.type)
                        // || (BG_DOOR_SCRIPT == cell->type) // Script doors aren't openable with bullets
                        || (BG_DOOR_MISSILE == cell->type && OBJ_BULLET_MISSILE == obj->c.type)
                        || (BG_DOOR_ICE == cell->type && OBJ_BULLET_ICE == obj->c.type)
                        || (BG_DOOR_XRAY == cell->type && OBJ_BULLET_XRAY == obj->c.type)
                        || (BG_DOOR_KEY_A == cell->type && KEY == ray->p.i.keys[rayMapId][0])
                        || (BG_DOOR_KEY_B == cell->type && KEY == ray->p.i.keys[rayMapId][1])
                        || (BG_DOOR_KEY_C == cell->type && KEY == ray->p.i.keys[rayMapId][2]))
                    {
                        // Start opening the door
                        cell->openingDirection = 1;
                        // Destroy this bullet
                        memset(obj, 0, sizeof(rayBullet_t));
                        obj->c.id = -1;

                        // If this door requires a key
                        if ((BG_DOOR_KEY_A == cell->type) || //
                            (BG_DOOR_KEY_B == cell->type) || //
                            (BG_DOOR_KEY_C == cell->type))
                        {
                            // Use the key
                            ray->p.i.keys[rayMapId][cell->type - BG_DOOR_KEY_A] = OPEN_KEY;
                        }
                    }
                    else
                    {
                        // Shot a closed door it couldn't open
                        // destroy this bullet
                        memset(obj, 0, sizeof(rayBullet_t));
                        obj->c.id = -1;
                    }
                }
            }
        }
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
    // Create a 'player' for collision comparison
    rayObjCommon_t player = {
        .posX   = ray->p.posX,
        .posY   = ray->p.posY,
        .radius = TO_FX_FRAC(32, 2 * TEX_WIDTH),
    };

    // Check if a bullet touches a player
    for (uint16_t bIdx = 0; bIdx < MAX_RAY_BULLETS; bIdx++)
    {
        rayBullet_t* bullet = &ray->bullets[bIdx];
        if (0 == bullet->c.id)
        {
            // An enemy's bullet
            if (objectsIntersect(&player, &bullet->c))
            {
                // TODO Player got shot, apply damage
                // De-allocate the bullet
                bullet->c.id = -1;
            }
        }
    }

    // Check if the player touches an item
    node_t* currentNode = ray->items.first;
    while (currentNode != NULL)
    {
        // Get a pointer from the linked list
        rayObjCommon_t* item = ((rayObjCommon_t*)currentNode->val);

        node_t* toRemove = NULL;
        // Check intersection
        if (objectsIntersect(&player, item))
        {
            // Touch the item
            rayPlayerTouchItem(ray, item->type, ray->p.mapId, item->id);
            // Check scripts
            if (checkScriptGet(ray, item->id, item->sprite))
            {
                // Script warped, return
                return;
            }

            // Mark this item for removal
            toRemove = currentNode;
        }

        // Iterate to the next node
        currentNode = currentNode->next;

        // If the prior node should be removed
        if (toRemove)
        {
            // Remove the lock
            if (ray->targetedObj == item)
            {
                ray->targetedObj = NULL;
            }
            // Free the item
            free(item);
            // Remove it from the list
            removeEntry(&ray->items, toRemove);
        }
    }

    // Check if a bullet touches an enemy
    currentNode = ray->enemies.first;
    while (currentNode != NULL)
    {
        // Get a pointer from the linked list
        rayEnemy_t* enemy = ((rayEnemy_t*)currentNode->val);
        // Used for iterating
        bool enemyWasKilled = false;

        // Iterate through all bullets
        for (uint16_t bIdx = 0; bIdx < MAX_RAY_BULLETS; bIdx++)
        {
            rayBullet_t* bullet = &ray->bullets[bIdx];
            if (1 == bullet->c.id)
            {
                // A player's bullet
                if (objectsIntersect(&enemy->c, &bullet->c))
                {
                    // De-allocate the bullet
                    bullet->c.id = -1;

                    // Enemy got shot, apply damage
                    if (rayEnemyGetShot(ray, enemy, bullet->c.type))
                    {
                        // Enemy was killed
                        checkScriptKill(ray, enemy->c.id, enemy->walkSprites[0]);

                        // save the next node
                        node_t* nextNode = currentNode->next;

                        // Remove the lock
                        if (ray->targetedObj == enemy)
                        {
                            ray->targetedObj = NULL;
                        }

                        // Unlink and free
                        removeEntry(&ray->enemies, currentNode);
                        free(enemy);

                        // Set the next node
                        currentNode    = nextNode;
                        enemyWasKilled = true;

                        break;
                    }
                }
            }
        }

        // If the enemy was killed, iteration already happened
        if (!enemyWasKilled)
        {
            // Iterate to the next node
            currentNode = currentNode->next;
        }
    }

    // Check if a bullet or the player touches scenery
    currentNode = ray->scenery.first;
    while (currentNode != NULL)
    {
        // Get a pointer from the linked list
        rayObjCommon_t* scenery = ((rayObjCommon_t*)currentNode->val);

        // Check if the player touches scenery
        if (objectsIntersect(&player, scenery))
        {
            if (checkScriptTouch(ray, scenery->id, scenery->sprite))
            {
                // Script warped, return
                return;
            }
        }

        // Iterate through all bullets
        for (uint16_t bIdx = 0; bIdx < MAX_RAY_BULLETS; bIdx++)
        {
            rayBullet_t* bullet = &ray->bullets[bIdx];
            if (1 == bullet->c.id)
            {
                // A player's bullet
                if (objectsIntersect(scenery, &bullet->c))
                {
                    // De-allocate the bullet
                    bullet->c.id = -1;

                    // Scenery was shot
                    if (checkScriptShootObjs(ray, scenery->id, scenery->sprite))
                    {
                        // Script warped, return
                        return;
                    }
                }
            }
        }

        // Iterate to the next node
        currentNode = currentNode->next;
    }
}
