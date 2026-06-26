#include "2d_renderer.h"
#include "ray_tex_manager.h"

void drawBackground2d(ray_t* ray, int32_t firstRow, int32_t lastRow)
{
    paletteColor_t* fb = getPxTftFramebuffer();
    for (uint32_t y = firstRow; y < lastRow; y++)
    {
        memset(&fb[y * TFT_WIDTH], c000, sizeof(paletteColor_t) * TFT_WIDTH);
    }
}

void drawCommonList(ray_t* ray, list_t* list, int camX, int camY)
{
    node_t* node = list->first;
    while (node)
    {
        rayObjCommon_t* obj = node->val;
        drawWsgSimple(obj->sprite, (obj->posX / 16) - camX - 8, obj->posY / 16 - camY - 8);
        node = node->next;
    }
}

void drawForeground2d(ray_t* ray)
{
    int camX = (ray->p.posX / 16) - (TFT_WIDTH / 2);
    int camY = (ray->p.posY / 16) - (TFT_HEIGHT / 2);

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

            drawWsgSimple(texture, mapX * 16 - camX, mapY * 16 - camY);
        }
    }

    drawCommonList(ray, &ray->enemies, camX, camY);
    drawCommonList(ray, &ray->scenery, camX, camY);
    drawCommonList(ray, &ray->items, camX, camY);

    for (int bIdx = 0; bIdx < MAX_RAY_BULLETS; bIdx++)
    {
        rayObjCommon_t* obj = &ray->bullets[bIdx].c;
        if (obj->type & BULLET)
        {
            drawWsgSimple(obj->sprite, (obj->posX / 16) - camX, obj->posY / 16 - camY);
        }
    }

    drawWsgSimple(&ray->cho_portrait, (ray->p.posX / 16) - camX - 8, (ray->p.posY / 16) - camY - 8);
}
