//==============================================================================
// Includes
//==============================================================================

#include "geometry.h"
#include "ray_object.h"
#include "ray_tex_manager.h"
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
bool checkBgCollision(ray_t* ray, q24_8 x, q24_8 y, rayMapCellType_t oType, int32_t oId);

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
 * @param accX The X acceleration of the bullet
 * @param accY The Y acceleration of the bullet
 * @param fuseUs The time before the bomb explodes, in microseconds, or negative to ignore
 * @param isPlayer true if this is the player's shot, false if it is an enemy's shot
 */
void rayCreateBullet(ray_t* ray, rayMapCellType_t bulletType, q24_8 posX, q24_8 posY, q24_8 velX, q24_8 velY,
                     q24_8 accX, q24_8 accY, int32_t fuseUs, bool isPlayer)
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
            wsg_t* texture              = getTexByType(ray, bulletType);
            newBullet->c.sprite         = texture;
            newBullet->c.spriteMirrored = false;
            newBullet->c.spriteRotation = 0;
            newBullet->c.solidColor     = cTransparent;
            if (OBJ_BULLET_BOMB == bulletType)
            {
                // Bombs start with a negative radius to not collide with enemies
                newBullet->c.bound.radius = -1;
            }
            else
            {
                // Width is based on the texture width as a fraction of a cell
                newBullet->c.bound.box.w = TO_FX_FRAC(newBullet->c.sprite->w, CELL_SIZE);
                newBullet->c.bound.box.h = TO_FX_FRAC(newBullet->c.sprite->h, CELL_SIZE);
                // Don't set radius
            }

            // Spawn it at the given position
            newBullet->c.posX = posX;
            newBullet->c.posY = posY;

            // Set the velocity
            newBullet->velX = velX;
            newBullet->velY = velY;

            // Set the velocity
            newBullet->accX = accX;
            newBullet->accY = accY;

            // Set the fuse
            newBullet->fuseUs = fuseUs;

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

    // Run a timer to open or close doors every 5ms
    RUN_TIMER_EVERY(ray->doorTimer, 5000, elapsedUs, {
        // Iterate the map
        for (int32_t y = 0; y < ray->map.h; y++)
        {
            for (int32_t x = 0; x < ray->map.w; x++)
            {
                // Get a reference to this cell
                rayMapCell_t* cell = &(ray->map.tiles[x][y]);

                // If the timer to start closing the door is running
                if (0 < cell->closeTimer)
                {
                    // Decrement it
                    cell->closeTimer--;
                    // If it expired
                    if (0 == cell->closeTimer)
                    {
                        // When this elapses, start closing the door
                        raySetDoorState(ray, x, y, false, false, false);
                    }
                }

                // If the door is opening
                if (0 < cell->openingDirection)
                {
                    // And it isn't fully open
                    if (cell->doorOpen < TO_FX(1))
                    {
                        // Open a little more
                        cell->doorOpen++;
                    }
                    else
                    {
                        // Door is fully open
                        cell->openingDirection = 0;

                        // Turn DOOR into FLOOR
                        cell->type = rayGetDefaultFloor();
                    }
                }
                // Else if the door is closing
                else if (0 > cell->openingDirection)
                {
                    // Make sure not to close on the player
                    if (x != FROM_FX(ray->p.posX) || y != FROM_FX(ray->p.posY))
                    {
                        // Close it a little more
                        if (cell->doorOpen > 0)
                        {
                            cell->doorOpen--;
                        }
                        else
                        {
                            // Door is fully closed
                            cell->openingDirection = 0;

                            // Turn FLOOR into DOOR
                            cell->type = rayGetDefaultDoor();
                        }
                    }
                }
            }
        }
    });
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
    // int32_t rayMapId = ray->p.mapId;

    // For each bullet slot
    for (uint32_t i = 0; i < MAX_RAY_BULLETS; i++)
    {
        // If a bullet is in the slot
        rayBullet_t* obj = &(ray->bullets[i]);
        if (-1 != obj->c.id)
        {
            // Save old velocity to check if the bullet stopped
            int32_t oldVelX = obj->velX;
            int32_t oldVelY = obj->velY;

            // If the boomerang is returning to the player
            if (obj->returnToPlayer)
            {
                // Point the velocity right back at the player
                obj->velX = (ray->p.posX - obj->c.posX);
                obj->velY = (ray->p.posY - obj->c.posY);
                fastNormVec(&obj->velX, &obj->velY);
                obj->velX /= 2;
                obj->velY /= 2;
            }
            else
            {
                // Update the bullet's velocity. (1 << 22) feels right
                obj->velX += (obj->accX * (int32_t)elapsedUs) / (1 << 22);
                obj->velY += (obj->accY * (int32_t)elapsedUs) / (1 << 22);

                // If the sign flipped, zero acceleration and velocity
                if ((oldVelX ^ obj->velX) < 0)
                {
                    obj->velX = 0;
                    obj->accX = 0;
                }

                if ((oldVelY ^ obj->velY) < 0)
                {
                    obj->velY = 0;
                    obj->accY = 0;
                }
            }

            // If the object stopped and there is no fuse
            if (obj->fuseUs < 0 && 0 == obj->velX && 0 == obj->velY)
            {
                // Destroy this bullet
                memset(obj, 0, sizeof(rayBullet_t));
                obj->c.id = -1;

                // Continue to the next
                continue;
            }

            // Update the bullet's position. (1 << 16) feels about right
            obj->c.posX += (obj->velX * (int32_t)elapsedUs) / (1 << 16);
            obj->c.posY += (obj->velY * (int32_t)elapsedUs) / (1 << 16);

            // Make sure the bullet is in bounds
            if (obj->c.posX < 0 || FROM_FX(obj->c.posX) >= ray->map.w || //
                obj->c.posY < 0 || FROM_FX(obj->c.posY) >= ray->map.h)
            {
                // Out of bounds, destroy this bullet
                memset(obj, 0, sizeof(rayBullet_t));
                obj->c.id = -1;
                continue;
            }

            // If there is a fuse
            if (obj->fuseUs >= 0)
            {
                // Decrement the fuse
                obj->fuseUs -= elapsedUs;

                // If the fuse elapsed
                if (obj->fuseUs < 0)
                {
                    // If this is a boomerang, return to player
                    if (OBJ_BULLET_BOOMERANG == obj->c.type)
                    {
                        obj->returnToPlayer = true;
                    }
                    // If this is a bomb, manage the explosion
                    else if (OBJ_BULLET_BOMB == obj->c.type)
                    {
                        // If the radius is negative
                        if (obj->c.bound.radius < 0)
                        {
                            // Explode by setting a positive radius
                            obj->fuseUs         = 100000;
                            obj->c.bound.radius = TO_FX(1);
                        }
                        else
                        {
                            // Explosion timeout, destroy this bullet
                            memset(obj, 0, sizeof(rayBullet_t));
                            obj->c.id = -1;

                            // Continue to the next
                            continue;
                        }
                    }
                }
            }

            // Run an animation timer for this bullet
            if (OBJ_BULLET_BOOMERANG == obj->c.type)
            {
                RUN_TIMER_EVERY(obj->animTimer, 1000000 / 8, elapsedUs, {
                    obj->c.spriteRotation += 90;
                    if (360 == obj->c.spriteRotation)
                    {
                        obj->c.spriteRotation = 0;
                    }
                });
            }

            if (checkBgCollision(ray, obj->c.posX, obj->c.posY, obj->c.type, obj->c.id))
            {
                // Destroy this bullet
                memset(obj, 0, sizeof(rayBullet_t));
                obj->c.id = -1;
            }

            // If this is a boomerang returning to the player, destroy it when it reaches the player
            if (OBJ_BULLET_BOOMERANG == obj->c.type && obj->returnToPlayer)
            {
                // Player cell
                vec_t pCell = {
                    .x = FROM_FX(ray->p.posX),
                    .y = FROM_FX(ray->p.posY),
                };

                // Boomerang cell
                vec_t objCell = {
                    .x = FROM_FX(obj->c.posX),
                    .y = FROM_FX(obj->c.posY),
                };

                // If they're the same cell
                if ((pCell.x == objCell.x) && (pCell.y == objCell.y))
                {
                    // Destroy this bullet
                    memset(obj, 0, sizeof(rayBullet_t));
                    obj->c.id = -1;
                }
            }
        }
    }
}

/**
 * @brief Check for collisions between an object and the background (walls and doors).
 * This checks scripts when objects hit walls, opens doors, etc.
 *
 * @param ray The entire game state
 * @param x The X location of the object
 * @param y The Y location of the object
 * @param oType The type of object
 * @param oId The object's ID
 * @return true if there was a collision, false if there wasn't
 */
bool checkBgCollision(ray_t* ray, q24_8 x, q24_8 y, rayMapCellType_t oType, int32_t oId)
{
    // Bombs don't collide with background
    if (OBJ_BULLET_BOMB == oType)
    {
        return false;
    }

    // Get the cell the bullet is in now
    rayMapCell_t* cell = &ray->map.tiles[FROM_FX(x)][FROM_FX(y)];

    // If the bullet hit something
    if (!isPassableCell(cell))
    {
        // If this is a player's bullet
        if (1 == oId)
        {
            // If it hit a wall
            if (CELL_IS_TYPE(cell->type, BG | WALL) && OBJ_BULLET_ARROW == oType)
            {
                // Check wall scripts
                checkScriptShootWall(ray, FROM_FX(x), FROM_FX(y));
            }
            // If it hit a door
            else if (CELL_IS_TYPE(cell->type, BG | DOOR))
            {
                // If the door is closed
                if (0 == cell->doorOpen)
                {
                    bool opened    = false;
                    bool keyOpened = false;
                    switch (cell->type)
                    {
                        case BG_DOOR_BUSH:
                        {
                            if (OBJ_BULLET_SWORD == oType)
                            {
                                // Slash the bush by chaning the type to floor
                                cell->type            = rayGetDefaultFloor();
                                rayMapCellType_t drop = rayEnemyStandardItemDrop();
                                if (EMPTY != drop)
                                {
                                    x &= 0xFFFFFF00;
                                    y &= 0xFFFFFF00;
                                    x += TO_FX_FRAC(1, 2);
                                    y += TO_FX_FRAC(1, 2);
                                    rayCreateCommonObj(ray, drop, 0x100, x, y);
                                }
                            }
                            break;
                        }
                        case BG_DOOR_CRACK_H:
                        case BG_DOOR_CRACK_V:
                        case BG_DOOR_ROCKS:
                        {
                            // Do nothing. Explosion radius checked against crackedWalls elsewhere
                            break;
                        }
                        case BG_DOOR_R_KEY_LOCKED:
                        case BG_DOOR_G_KEY_LOCKED:
                        case BG_DOOR_B_KEY_LOCKED:
                        case BG_DOOR_K_KEY_LOCKED:
                        {
                            // Check if the player has an unused key that matches
                            for (int idx = 0; idx < ARRAY_SIZE(ray->p.i.items); idx++)
                            {
                                invItem_t* invItem = &ray->p.i.items[idx];
                                if (invItem->occupied &&                            // Has item
                                    invItem->mapId == ray->p.mapId &&               // in this map
                                    (((invItem->type == (OBJ_ITEM_R_KEY & ID_MASK)) // And type matches the door
                                      && (BG_DOOR_R_KEY_LOCKED == cell->type))
                                     || ((invItem->type == (OBJ_ITEM_G_KEY & ID_MASK))
                                         && (BG_DOOR_G_KEY_LOCKED == cell->type))
                                     || ((invItem->type == (OBJ_ITEM_B_KEY & ID_MASK))
                                         && (BG_DOOR_B_KEY_LOCKED == cell->type))
                                     || ((invItem->type == (OBJ_ITEM_K_KEY & ID_MASK))
                                         && (BG_DOOR_K_KEY_LOCKED == cell->type)))
                                    && !invItem->keyUsed) // not used yet
                                {
                                    // Use the key
                                    invItem->keyUsed = true;

                                    switch (cell->type)
                                    {
                                        case BG_DOOR_R_KEY_LOCKED:
                                        {
                                            ray->ps.keyMask.r_key = false;
                                            break;
                                        }
                                        case BG_DOOR_G_KEY_LOCKED:
                                        {
                                            ray->ps.keyMask.g_key = false;
                                            break;
                                        }
                                        case BG_DOOR_B_KEY_LOCKED:
                                        {
                                            ray->ps.keyMask.b_key = false;
                                            break;
                                        }
                                        case BG_DOOR_K_KEY_LOCKED:
                                        {
                                            ray->ps.keyMask.k_key = false;
                                            break;
                                        }
                                    }
                                    // Open the door
                                    opened    = true;
                                    keyOpened = true;
                                    break;
                                }
                            }
                            break;
                        }
                        case BG_DOOR_DUNGEON:
                        {
                            opened = true;
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
                        raySetDoorState(ray, FROM_FX(x), FROM_FX(y), true, keyOpened, keyOpened);

                        // Autosave when a key is used
                        if (keyOpened)
                        {
                            raySaveGame(ray);
                        }
                    }
                }
            }
        }

        // Hit something
        return true;
    }

    // Didn't hit something
    return false;
}

/**
 * @brief Get the bounding box for an object
 *
 * @param obj
 * @return rectangle_t
 */
rectangle_t rayGetObjBB(const rayObjCommon_t* obj)
{
    rectangle_t bb = {
        .pos.x  = obj->posX - (obj->bound.box.w / 2),
        .pos.y  = obj->posY - (obj->bound.box.h / 2),
        .width  = obj->bound.box.w,
        .height = obj->bound.box.h,
    };
    return bb;
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
    return rectRectIntersection(rayGetObjBB(obj1), rayGetObjBB(obj2), NULL);
}

/**
 * @brief Check for collisions between bullets, enemies, and the player
 *
 * @param ray The entire game state
 */
void checkRayCollisions(ray_t* ray)
{
    // Create a 'player' for collision comparison
    rectangle_t pbb       = rayGetPlayerBB(ray);
    rayObjCommon_t player = {
        .posX        = pbb.pos.x + (pbb.width / 2),
        .posY        = pbb.pos.y + (pbb.height / 2),
        .bound.box.w = pbb.width,
        .bound.box.h = pbb.height,
    };

    // Check if a bullet touches a player or cracked wall
    for (uint16_t bIdx = 0; bIdx < MAX_RAY_BULLETS; bIdx++)
    {
        rayBullet_t* bullet = &ray->bullets[bIdx];
        if (0 == bullet->c.id)
        {
            // An enemy's bullet
            if (objectsIntersect(&player, &bullet->c))
            {
                // Determine the damage per-bullet and if the bullet gets deleted or reflected
                int32_t dmg       = 0;
                bool deleteBullet = true;
                switch (bullet->c.type)
                {
                    case OBJ_BULLET_SHIELD_0:
                    case OBJ_BULLET_SHIELD_1:
                    case OBJ_BULLET_SHIELD_2:
                    case OBJ_BULLET_SHIELD_3:
                    {
                        // Assume 1 damage for now
                        dmg = 1;

                        // If the shield is active
                        if (ray->ps.shieldTimerUs)
                        {
                            if (ray->ps.shieldZone == (bullet->c.type - OBJ_BULLET_SHIELD_0))
                            {
                                // No damage
                                dmg = 0;

                                // Don't delete bullet
                                deleteBullet = false;

                                // Reflect the bullet
                                bullet->velX = -bullet->velX;
                                bullet->velY = -bullet->velY;

                                // Player takes control of bullet
                                bullet->c.id = 1;
                            }
                            else
                            {
                                // No damage, but no reflection either
                                dmg = 0;
                            }
                        }
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }

                // Player got shot, apply damage
                rayPlayerDecrementHealth(ray, dmg);

                if (deleteBullet)
                {
                    // De-allocate the bullet
                    memset(bullet, 0, sizeof(rayBullet_t));
                    bullet->c.id = -1;
                }
            }
        }
        else if (OBJ_BULLET_BOMB == bullet->c.type && bullet->c.bound.radius > 0) // Player's exploded bomb
        {
            // Make a circle for the explosion
            circle_t bomb = {
                .pos = {
                    .x = bullet->c.posX,
                    .y = bullet->c.posY,
                },
                .radius = bullet->c.bound.radius,
            };

            // Iterate through cracked walls
            node_t* cNode = ray->map.crackedWalls.first;
            while (cNode)
            {
                uint16_t wallX     = (((intptr_t)cNode->val) >> 16) & 0xFFFF;
                uint16_t wallY     = ((intptr_t)cNode->val) & 0xFFFF;
                rayMapCell_t* cell = &ray->map.tiles[wallX][wallY];

                // Make a rectangle for the wall
                rectangle_t wall = {
                    .pos = {
                        .x = TO_FX(wallX),
                        .y = TO_FX(wallY),
                    },
                    .height = TO_FX(1),
                    .width = TO_FX(1),
                };

                // If the explosion intersects the wall
                if (circleRectIntersection(bomb, wall, NULL))
                {
                    // open the door by changing the floor type
                    // TODO door animation
                    cell->type = rayGetDefaultFloor();

                    // Remove this address from the list
                    node_t* next = cNode->next;
                    removeEntry(&ray->map.crackedWalls, cNode);
                    cNode = next;
                }
                else
                {
                    // Iterate normally
                    cNode = cNode->next;
                }
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
            rayPlayerTouchItem(ray, item, ray->p.mapId);
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
            // Free the item
            heap_caps_free(item);
            // Remove it from the list
            removeEntry(&ray->items, toRemove);
        }
    }

    // Get a sword line segment
    line_t sword = rayGetSwordLineSegment(ray);

    // Only check the sword if it's active
    if (ray->ps.swordActive)
    {
        // Check for sword / background collisions
        checkBgCollision(ray, sword.p1.x, sword.p1.y, OBJ_BULLET_SWORD, 1);
        checkBgCollision(ray, sword.p2.x, sword.p2.y, OBJ_BULLET_SWORD, 1);
        // Don't set swordActive to false so that multiple bushes can be slashed
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
            if (1 == bullet->c.id && bullet->c.bound.radius > 0)
            {
                // A player's bullet
                if (objectsIntersect(&enemy->c, &bullet->c))
                {
                    // Decrease HP based on the shot and enemy type
                    rayEnemyGetShot(ray, enemy, bullet->c.type);

                    // de-spawn (and sword)
                    if (bullet->fuseUs < 0)
                    {
                        // De-allocate the bullet
                        memset(bullet, 0, sizeof(rayBullet_t));
                        bullet->c.id = -1;
                    }
                }
            }
        }

        // If a sword is being swung
        if (ray->ps.swordActive > 0)
        {
            // Get the enemy bounding box
            // Check for a collision between bounding box and sword
            if (rectLineIntersection(rayGetObjBB(&enemy->c), sword, NULL))
            {
                rayEnemyGetShot(ray, enemy, OBJ_BULLET_SWORD);
                // Stop the sword swing
                ray->ps.swordActive = false;
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
                    memset(bullet, 0, sizeof(rayBullet_t));
                    bullet->c.id = -1;

                    // Scenery was shot
                    checkScriptShootObjs(ray, scenery->id, scenery->sprite);
                }
            }
        }

        // If a sword is being swung
        if (ray->ps.swordActive > 0)
        {
            // Check if the sword touches scenery
            if (rectLineIntersection(rayGetObjBB(scenery), sword, NULL))
            {
                // Check
                if (checkScriptShootObjs(ray, scenery->id, scenery->sprite))
                {
                    // Stop the sword swing
                    ray->ps.swordActive = false;
                }
            }
        }

        // Iterate to the next node
        currentNode = currentNode->next;
    }
}

/**
 * @brief Check if an object's bounding box fits in the map by checking if all four corners are in passable cells
 *
 * @param ray The entire game state
 * @param bb The bounding box
 * @return true if the bounding box exists entirely in passable cells, false otherwise
 */
bool rayBoundingBoxFitsInMap(ray_t* ray, rectangle_t bb)
{
    // Find the cells for the four corners of the bounding box
    int32_t x1 = FROM_FX(bb.pos.x);
    int32_t x2 = FROM_FX(bb.pos.x + bb.width);
    int32_t y1 = FROM_FX(bb.pos.y);
    int32_t y2 = FROM_FX(bb.pos.y + bb.height);

    // Check if each corner is in a passable cell
    return isPassableCell(&ray->map.tiles[x1][y1]) && //
           isPassableCell(&ray->map.tiles[x1][y2]) && //
           isPassableCell(&ray->map.tiles[x2][y1]) && //
           isPassableCell(&ray->map.tiles[x2][y2]);
}

/**
 * @brief Open or close the door on the given cell
 *
 * @param ray The entire game state
 * @param x The X position of the door tile
 * @param y The Y position of the door tile
 * @param isOpening True to open the door, false to close it
 * @param setScriptOpen True to mark the door as permanently open, false to not
 * @param playSfx True to play a jingle, false to stay silent
 */
void raySetDoorState(ray_t* ray, uint32_t x, uint32_t y, bool isOpening, bool setScriptOpen, bool playSfx)
{
    int8_t direction = isOpening ? 1 : -1;

    // Open or close the given door
    ray->map.tiles[x][y].openingDirection = direction;

    // Do additional things if the door is opening
    if (isOpening)
    {
        if (setScriptOpen)
        {
            ray->map.visitedTiles[(y * ray->map.w) + x] = SCRIPT_DOOR_OPEN;
        }

        if (playSfx)
        {
            // Play SFX
            globalMidiPlayerPlaySong(&ray->sfx_door_open, MIDI_SFX);
        }
    }

    // Also check cardinal adjacent doors
    vec_t adj[4] = {
        {.x = x - 1, .y = y},
        {.x = x + 1, .y = y},
        {.x = x, .y = y - 1},
        {.x = x, .y = y + 1},
    };

    // For each adjacent tile
    for (uint32_t i = 0; i < ARRAY_SIZE(adj); i++)
    {
        // If it's in-bounds
        if (0 <= adj[i].x && adj[i].x < ray->map.w && //
            0 <= adj[i].y && adj[i].y < ray->map.h)
        {
            // If the cell is a door
            rayMapCell_t* cell = &ray->map.tiles[adj[i].x][adj[i].y];
            if (CELL_IS_TYPE(cell->type, BG | DOOR))
            {
                cell->openingDirection = direction;

                if (isOpening && setScriptOpen)
                {
                    ray->map.visitedTiles[(adj[i].y * ray->map.w) + adj[i].x] = SCRIPT_DOOR_OPEN;
                }
            }
        }
    }
}
