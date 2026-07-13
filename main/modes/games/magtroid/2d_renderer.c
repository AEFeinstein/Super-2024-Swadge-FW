#include "fp_math.h"
#include "2d_renderer.h"
#include "ray_tex_manager.h"
#include "ray_player.h"
#include "ray_enemy.h"

#define TO_PX(x) ((CELL_SIZE * (x)) / 256)

void drawBackground2d(ray_t* ray, int32_t firstRow, int32_t lastRow)
{
    paletteColor_t* fb = getPxTftFramebuffer();
    for (uint32_t y = firstRow; y < lastRow; y++)
    {
        memset(&fb[y * TFT_WIDTH], c000, sizeof(paletteColor_t) * TFT_WIDTH);
    }
}

void drawCommonList(ray_t* ray, list_t* list, int camX, int camY, bool drawBB)
{
    node_t* node = list->first;
    while (node)
    {
        rayObjCommon_t* obj = node->val;
        drawWsgSimple(obj->sprite,                               //
                      TO_PX(obj->posX) - camX - (CELL_SIZE / 2), //
                      TO_PX(obj->posY) - camY - (CELL_SIZE / 2));

        if (drawBB)
        {
            rectangle_t bb = rayGetEnemyBoundingBox(node->val);
            drawRect(TO_PX(bb.pos.x) - camX,            //
                     TO_PX(bb.pos.y) - camY,            //
                     TO_PX(bb.pos.x + bb.width) - camX, //
                     TO_PX(bb.pos.y + bb.height) - camY, c005);
        }
        node = node->next;
    }
}

void drawForeground2d(ray_t* ray)
{
    int32_t camX = ray->camera.x;
    int32_t camY = ray->camera.y;

    for (int mapY = 0; mapY < ray->map.h; mapY++)
    {
        for (int mapX = 0; mapX < ray->map.w; mapX++)
        {
            // Get the next cell texture
            rayMapCellType_t type = ray->map.tiles[mapX][mapY].type;
            wsg_t* texture;

            // These are generic
            if ((BG_FLOOR_LAVA == type) || (BG_FLOOR_WATER == type) || (BG_FLOOR_HEAL == type)
                || (BG_DOOR <= type && type <= BG_DOOR_ARTIFACT))
            {
                texture = getTexByType(ray, type);
            }
            // These are environment specific
            else if (BG_WALL_1 <= type && type <= BG_WALL_5)
            {
                texture = &ray->envTex[ray->p.mapId % NUM_ENVS][type - BG_WALL_1];
            }
            // Floor is also environment specific
            else
            {
                texture = &ray->envTex[ray->p.mapId % NUM_ENVS][TX_FLOOR];
            }

            drawWsgTile(texture, mapX * CELL_SIZE - camX, mapY * CELL_SIZE - camY);
        }
    }

    drawCommonList(ray, &ray->enemies, camX, camY, true);
    drawCommonList(ray, &ray->scenery, camX, camY, false);
    drawCommonList(ray, &ray->items, camX, camY, false);

    for (int bIdx = 0; bIdx < MAX_RAY_BULLETS; bIdx++)
    {
        rayObjCommon_t* obj = &ray->bullets[bIdx].c;
        if (obj->type & BULLET && obj->id >= 0)
        {
            drawWsgSimple(obj->sprite, TO_PX(obj->posX) - camX, TO_PX(obj->posY) - camY);
        }
    }

    drawWsg(&ray->cho_portrait, TO_PX(ray->p.posX) - camX - (CELL_SIZE / 2),
            TO_PX(ray->p.posY) - camY - (CELL_SIZE / 2), false, false, ray->p.dirAngle);

    if (ray->p.swordTimerUs > 0)
    {
        line_t sword = rayGetSwordLineSegment(ray);
        drawLineFast(TO_PX(sword.p1.x) - camX, //
                     TO_PX(sword.p1.y) - camY, //
                     TO_PX(sword.p2.x) - camX, //
                     TO_PX(sword.p2.y) - camY, c550);
    }
}
