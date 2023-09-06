//==============================================================================
// Includes
//==============================================================================

#include "ray_object.h"
#include "ray_tex_manager.h"
#include "ray_renderer.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static bool objectsIntersect(const rayObj_t* obj1, const rayObj_t* obj2);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Check if two ::rayObj_t intersect
 *
 * @param obj1 The first rayObj_t to check for intersection
 * @param obj2 The second rayObj_t to check for intersection
 * @return true if they intersect, false if they do not
 */
static bool objectsIntersect(const rayObj_t* obj1, const rayObj_t* obj2)
{
    q24_8 deltaX    = (obj2->posX - obj1->posX);
    q24_8 deltaY    = (obj2->posY - obj1->posY);
    q24_8 radiusSum = (obj1->radius + obj2->radius);
    return (deltaX * deltaX) + (deltaY * deltaY) < (radiusSum * radiusSum);
}

/**
 * @brief Initialize a bullet fired from the player's position in the player's direction based on the player's loadout
 *
 * @param ray The entire game state
 */
void fireShot(ray_t* ray)
{
    // Iterate over the bullet list, finding a new slot
    for (uint16_t newIdx = 0; newIdx < MAX_RAY_BULLETS; newIdx++)
    {
        // If this slot has a negative ID, use it
        if (-1 == ray->bullets[newIdx].id)
        {
            // Get a convenience pointer
            rayObj_t* newBullet = &ray->bullets[newIdx];

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
            newBullet->type   = bulletType;
            wsg_t* texture    = getTexByType(ray, bulletType);
            newBullet->sprite = texture;
            // Spawn it slightly in front of the player
            newBullet->posX = ray->posX + ray->dirX / 2;
            newBullet->posY = ray->posY + ray->dirY / 2;
            // Velocity is the player's direction
            newBullet->velX = ray->dirX;
            newBullet->velY = ray->dirY;

            // Width is based on the texture width as a fraction of a cell
            newBullet->radius = TO_FX_FRAC(texture->w, TEX_WIDTH);
            // Bullets don't have real IDs
            newBullet->id = 0;

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
void moveRayObjects(ray_t* ray, int64_t elapsedUs)
{
    // For each bullet slot
    for (uint16_t i = 0; i < MAX_RAY_BULLETS; i++)
    {
        // If a bullet is in the slot
        rayObj_t* obj = &(ray->bullets[i]);
        if (0 == obj->id)
        {
            // Update the bullet's position
            // TODO justify the scaling factor
            obj->posX += (obj->velX * elapsedUs) / 100000;
            obj->posY += (obj->velY * elapsedUs) / 100000;

            // Get the cell the bullet is in now
            rayMapCell_t* cell = &ray->map.tiles[FROM_FX(obj->posX)][FROM_FX(obj->posY)];

            // If a wall is it
            if (CELL_IS_TYPE(cell->type, BG | WALL))
            {
                // Destroy this bullet
                memset(obj, 0, sizeof(rayObj_t));
                obj->id = -1;
            }
            // Else if a door is hit
            else if (CELL_IS_TYPE(cell->type, BG | DOOR))
            {
                // Check if the bullet type can open the door
                // TODO handle charge beam
                if ((BG_DOOR == cell->type) // Normal doors are openable by anything
                    || (BG_DOOR_CHARGE == cell->type /*&& OBJ_BULLET_CHARGE == obj->type*/)
                    || (BG_DOOR_MISSILE == cell->type && OBJ_BULLET_MISSILE == obj->type)
                    || (BG_DOOR_ICE == cell->type && OBJ_BULLET_ICE == obj->type)
                    || (BG_DOOR_XRAY == cell->type && OBJ_BULLET_XRAY == obj->type))
                {
                    // If the door is closed
                    if (0 == cell->doorOpen)
                    {
                        // Start opening the door
                        cell->doorOpen = 1;
                        // Destroy this bullet
                        memset(obj, 0, sizeof(rayObj_t));
                        obj->id = -1;
                    }
                }
            }
        }
    }
    // TODO move all enemies
    // TODO check for bullet-object collisions after updating all positions
}
