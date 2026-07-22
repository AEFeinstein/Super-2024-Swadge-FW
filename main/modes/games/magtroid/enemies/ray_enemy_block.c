#include "ray_enemy_block.h"
#include "ray_object.h"

void rayEnemyBlockCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
{
    rectangle_t enemyBB = rayGetObjBB(&enemy->c);
    if (rectRectIntersection(player, enemyBB, NULL))
    {
        // Only move the box when the player's movement is axis aligned
        if (((*deltaX) && !(*deltaY)) || (!(*deltaX) && (*deltaY)))
        {
            // Try moving the bounding box and check if it fits
            enemyBB.pos.x += *deltaX;
            enemyBB.pos.y += *deltaY;
            if (rayBoundingBoxFitsInMap(ray, enemyBB))
            {
                // If it fits, update the position
                enemy->c.posX += *deltaX;
                enemy->c.posY += *deltaY;
            }
            else
            {
                // If it doesn't fit, stop the player's movement
                *deltaX = 0;
                *deltaY = 0;
            }
        }
        else if (*deltaX && *deltaY)
        {
            // Stop the player from moving on diagonals
            *deltaX = 0;
            *deltaY = 0;
        }
    }
}