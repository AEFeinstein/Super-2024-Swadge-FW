#include "ray_enemy_block.h"
#include "ray_object.h"

void rayEnemyBlockCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
{
    if (rectRectIntersection(player, rayGetObjBB(&enemy->c), NULL))
    {
        // Only move the box when the player's movement is axis aligned
        if (((*deltaX) && !(*deltaY)) || (!(*deltaX) && (*deltaY)))
        {
            if (true) // TODO If the box can move to the destination
            {
                enemy->c.posX += *deltaX;
                enemy->c.posY += *deltaY;
            }
            else
            {
                // If the player can't move the box can't move
                *deltaX = 0;
                *deltaY = 0;
            }
        }
        else if (*deltaX && *deltaY)
        {
            // Stop the player fromm moving on diagonals
            *deltaX = 0;
            *deltaY = 0;
        }
    }
}