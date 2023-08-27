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
        switch (obj->type)
        {
            case BULLET_NORMAL:
            {
                // Update the bullet's position
                // TODO justify the scaling factor
                obj->posX += (obj->velX * elapsedUs) / 100000;
                obj->posY += (obj->velY * elapsedUs) / 100000;

                // Check if it hit a wall
                rayMapCell_t* cell = &ray->map.tiles[FROM_FX(obj->posX)][FROM_FX(obj->posY)];
                switch (cell->type)
                {
                    case BG_WALL_1:
                    case BG_WALL_2:
                    case BG_WALL_3:
                    {
                        // Destroy this bullet
                        memset(obj, 0, sizeof(rayObj_t));
                        obj->id = -1;
                        break;
                    }
                    case BG_DOOR:
                    case BG_DOOR_CHARGE:
                    case BG_DOOR_MISSILE:
                    case BG_DOOR_ICE:
                    case BG_DOOR_XRAY:
                    {
                        // Start opening the door
                        if (0 == cell->doorOpen)
                        {
                            cell->doorOpen = 1;
                            // Destroy this bullet
                            memset(obj, 0, sizeof(rayObj_t));
                            obj->id = -1;
                        }
                        break;
                    }
                    case EMPTY:
                    case BG_FLOOR:
                    case BG_FLOOR_WATER:
                    case BG_FLOOR_LAVA:
                    case OBJ_START_POINT:
                    case OBJ_ENEMY_BEAM:
                    case OBJ_ENEMY_CHARGE:
                    case OBJ_ENEMY_MISSILE:
                    case OBJ_ENEMY_ICE:
                    case OBJ_ENEMY_XRAY:
                    case OBJ_ITEM_BEAM:
                    case OBJ_ITEM_CHARGE_BEAM:
                    case OBJ_ITEM_MISSILE:
                    case OBJ_ITEM_ICE:
                    case OBJ_ITEM_XRAY:
                    case OBJ_ITEM_SUIT_WATER:
                    case OBJ_ITEM_SUIT_LAVA:
                    case OBJ_ITEM_ENERGY_TANK:
                    case OBJ_ITEM_KEY:
                    case OBJ_ITEM_ARTIFACT:
                    case OBJ_ITEM_PICKUP_ENERGY:
                    case OBJ_ITEM_PICKUP_MISSILE:
                    case OBJ_SCENERY_TERMINAL:
                    case OBJ_DELETE:
                    case BULLET_NORMAL:
                    case BULLET_CHARGE:
                    case BULLET_ICE:
                    case BULLET_MISSILE:
                    case BULLET_XRAY:
                    case BG_CEILING:
                    case NUM_RAY_MAP_CELL_TYPES:
                    {
                        // No collision
                        break;
                    }
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }

    // TODO check for bullet-object collisions after updating all positions
}
