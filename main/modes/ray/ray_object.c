#include "ray_object.h"

static bool objectsIntersect(rayObj_t* obj1, rayObj_t* obj2);

static bool objectsIntersect(rayObj_t* obj1, rayObj_t* obj2)
{
    q24_8 deltaX    = (obj2->posX - obj1->posX);
    q24_8 deltaY    = (obj2->posY - obj1->posY);
    q24_8 radiusSum = (obj1->radius + obj2->radius);
    return (deltaX * deltaX) + (deltaY * deltaY) < (radiusSum * radiusSum);
}

void moveRayObjects(ray_t* ray, int64_t elapsedUs)
{
    for (uint16_t i = 0; i < MAX_RAY_OBJS; i++)
    {
        rayObj_t* obj = &(ray->objs[i]);
        switch (obj->type)
        {
            case OBJ_BULLET:
            {
                // Update the bullet's position
                // TODO justify the scaling factor
                obj->posX += (obj->velX * elapsedUs) / 1000000;
                obj->posY += (obj->velY * elapsedUs) / 1000000;

                // Check if it hit a wall
                switch (ray->map.tiles[FROM_FX(obj->posX)][FROM_FX(obj->posX)])
                {
                    case BG_WALL:
                    {
                        // TODO destroy this bullet
                        memset(obj, 0, sizeof(rayObj_t));
                        obj->id = -1;
                        break;
                    }
                    case BG_DOOR:
                    {
                        // TODO check if door is open
                        break;
                    }
                    case EMPTY:
                    case BG_FLOOR:
                    case BG_CEILING:
                    case OBJ_START_POINT:
                    case OBJ_ENEMY_DRAGON:
                    case OBJ_ENEMY_SKELETON:
                    case OBJ_ENEMY_KNIGHT:
                    case OBJ_ENEMY_GOLEM:
                    case OBJ_OBELISK:
                    case OBJ_GUN:
                    case OBJ_BULLET:
                    case OBJ_DELETE:
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