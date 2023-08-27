#include "ray_object.h"

static bool objectsIntersect(rayObj_t* obj1, rayObj_t* obj2);

/**
 * @brief TODO doc
 *
 * @param obj1
 * @param obj2
 * @return true
 * @return false
 */
static bool objectsIntersect(rayObj_t* obj1, rayObj_t* obj2)
{
    q24_8 deltaX    = (obj2->posX - obj1->posX);
    q24_8 deltaY    = (obj2->posY - obj1->posY);
    q24_8 radiusSum = (obj1->radius + obj2->radius);
    return (deltaX * deltaX) + (deltaY * deltaY) < (radiusSum * radiusSum);
}

/**
 * @brief TODO doc
 *
 * @param ray
 * @param elapsedUs
 */
void moveRayObjects(ray_t* ray, int64_t elapsedUs)
{
    for (uint16_t i = 0; i < MAX_RAY_OBJS; i++)
    {
        rayObj_t* obj = &(ray->objs[i]);

        if (CELL_IS_TYPE(obj->type, OBJ | BULLET))
        {
            // Update the bullet's position
            // TODO justify the scaling factor
            obj->posX += (obj->velX * elapsedUs) / 100000;
            obj->posY += (obj->velY * elapsedUs) / 100000;

            // Check if it hit a wall
            rayMapCell_t* cell = &ray->map.tiles[FROM_FX(obj->posX)][FROM_FX(obj->posY)];
            switch (cell->type)
            {
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
                    // Start opening the door
                    // TODO check bullet / door combo
                    if (0 == cell->doorOpen)
                    {
                        cell->doorOpen = 1;
                        // Destroy this bullet
                        memset(obj, 0, sizeof(rayObj_t));
                        obj->id = -1;
                    }
                }
            }
            break;
        }
    }
    // TODO check for bullet-object collisions after updating all positions
}
