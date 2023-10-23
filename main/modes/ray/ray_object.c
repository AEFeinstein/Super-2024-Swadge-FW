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
static void moveRayBullets(ray_t* ray, uint32_t elapsedUs);

//==============================================================================
// Functions
//==============================================================================

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
void moveRayObjects(ray_t* ray, uint32_t elapsedUs)
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
static void moveRayBullets(ray_t* ray, uint32_t elapsedUs)
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
            obj->c.posX += (obj->velX * (int32_t)elapsedUs) / 100000;
            obj->c.posY += (obj->velY * (int32_t)elapsedUs) / 100000;

            // Get the cell the bullet is in now
            rayMapCell_t* cell = &ray->map.tiles[FROM_FX(obj->c.posX)][FROM_FX(obj->c.posY)];

            // If the bullet hit something
            if (!isPassableCell(cell))
            {
                // If this is a player's bullet
                if (1 == obj->c.id)
                {
                    // If it hit a wall
                    if (CELL_IS_TYPE(cell->type, BG | WALL))
                    {
                        // Check wall scripts
                        checkScriptShootWall(ray, FROM_FX(obj->c.posX), FROM_FX(obj->c.posY));
                    }
                    // If it hit a door
                    else if (CELL_IS_TYPE(cell->type, BG | DOOR))
                    {
                        // If the door is closed
                        if (0 == cell->doorOpen)
                        {
                            bool opened = false;
                            switch (cell->type)
                            {
                                case BG_DOOR:
                                {
                                    opened = true;
                                    break;
                                }
                                case BG_DOOR_CHARGE:
                                {
                                    opened = (OBJ_BULLET_CHARGE == obj->c.type);
                                    break;
                                }
                                case BG_DOOR_MISSILE:
                                {
                                    opened = (OBJ_BULLET_MISSILE == obj->c.type);
                                    break;
                                }
                                case BG_DOOR_ICE:
                                {
                                    opened = (OBJ_BULLET_ICE == obj->c.type);
                                    break;
                                }
                                case BG_DOOR_XRAY:
                                {
                                    opened = (OBJ_BULLET_XRAY == obj->c.type);
                                    break;
                                }
                                case BG_DOOR_SCRIPT:
                                {
                                    // Script doors aren't openable by bullets
                                    break;
                                }
                                case BG_DOOR_KEY_A:
                                case BG_DOOR_KEY_B:
                                case BG_DOOR_KEY_C:
                                {
                                    // Open the door if the player has the appropriate key
                                    opened = (KEY == ray->p.i.keys[rayMapId][cell->type - BG_DOOR_KEY_A]);
                                    if (opened)
                                    {
                                        // Mark the key as used
                                        ray->p.i.keys[rayMapId][cell->type - BG_DOOR_KEY_A] = OPEN_KEY;
                                    }
                                    break;
                                }
                                case BG_DOOR_ARTIFACT:
                                {
                                    // Check if all artifacts have been collected
                                    opened = true;
                                    for (int16_t aIdx = 0; aIdx < ARRAY_SIZE(ray->p.i.artifacts); aIdx++)
                                    {
                                        if (!ray->p.i.artifacts[aIdx])
                                        {
                                            opened = false;
                                            break;
                                        }
                                    }
                                    break;
                                }
                                default:
                                {
                                    // Not a door, somehow
                                    break;
                                }
                            }

                            // If the door was opened
                            if (opened)
                            {
                                // Start opening the door
                                cell->openingDirection = 1;
                            }
                        }
                    }
                }

                // Destroy this bullet
                memset(obj, 0, sizeof(rayBullet_t));
                obj->c.id = -1;
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
                rayPlayerDecrementHealth(ray, 1);
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
            checkScriptGet(ray, item->id, item->sprite);

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

        // Iterate through all bullets
        for (uint16_t bIdx = 0; bIdx < MAX_RAY_BULLETS; bIdx++)
        {
            rayBullet_t* bullet = &ray->bullets[bIdx];
            if (1 == bullet->c.id)
            {
                // A player's bullet
                if (objectsIntersect(&enemy->c, &bullet->c))
                {
                    // Decrease HP based on the shot and enemy type
                    rayEnemyGetShot(ray, enemy, bullet->c.type);
                    // De-allocate the bullet
                    bullet->c.id = -1;
                }
            }
        }

        // Iterate to the next
        currentNode = currentNode->next;
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
            checkScriptTouch(ray, scenery->id, scenery->sprite);
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
                    checkScriptShootObjs(ray, scenery->id, scenery->sprite);
                }
            }
        }

        // Iterate to the next node
        currentNode = currentNode->next;
    }
}
