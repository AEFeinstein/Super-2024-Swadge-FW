#include "ray_enemy_box.h"
#include "ray_object.h"
#include "ray_tex_manager.h"
#include "ray_script.h"

void rayEnemyBoxCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY);

void rayInitEnemyBox(ray_t* ray, rayEnemy_t* e)
{
    // Box has one texture
    e->c.sprite = loadTexture(ray, OBJ_ENEMY_BOX_WSG, OBJ_ENEMY_BOX);

    // Box has 1hp to not die
    e->health = 1;

    // No enemy-specific state
    e->state = NULL;

    // Set function pointers
    e->mainFn      = NULL;
    e->collisionFn = rayEnemyBoxCheckPlayerCollision;
    e->getShotFn   = NULL;
}

void rayEnemyBoxCheckPlayerCollision(ray_t* ray, rayEnemy_t* enemy, rectangle_t player, q24_8* deltaX, q24_8* deltaY)
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

                vec_t oldCell = {
                    .x = FROM_FX(enemy->c.posX),
                    .y = FROM_FX(enemy->c.posY),
                };

                enemy->c.posX += *deltaX;
                enemy->c.posY += *deltaY;

                vec_t newCell = {
                    .x = FROM_FX(enemy->c.posX),
                    .y = FROM_FX(enemy->c.posY),
                };

                if (oldCell.x != newCell.x || oldCell.y != newCell.y)
                {
                    checkScriptObjEnter(ray, enemy->c.id, newCell.x, newCell.y, enemy->c.sprite);
                }
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