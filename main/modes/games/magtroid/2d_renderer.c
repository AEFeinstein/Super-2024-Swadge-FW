#include "fp_math.h"
#include "2d_renderer.h"
#include "ray_tex_manager.h"
#include "ray_player.h"
#include "ray_enemy.h"

#define TO_PX(x) ((CELL_SIZE * (x)) / 256)

void drawCommonList(ray_t* ray, list_t* list, int camX, int camY, paletteColor_t bbColor);

void drawBackground2d(ray_t* ray, int32_t firstRow, int32_t lastRow)
{
    // Get the framebuffer at this row
    paletteColor_t* fb = getPxTftFramebuffer() + (TFT_WIDTH * firstRow);

    // Find the row and offset into the texture to start at
    int32_t mapY       = (ray->camera.y + firstRow) / CELL_SIZE;
    int32_t texOffsetY = (ray->camera.y + firstRow) % CELL_SIZE;

    // Find the column and offset into the texture to start at
    int32_t iMapX       = (ray->camera.x) / CELL_SIZE;
    int32_t iTexOffsetX = (ray->camera.x) % CELL_SIZE;
    int32_t iCopySize   = CELL_SIZE - iTexOffsetX;

    // For each pixel row in this update
    for (uint32_t y = firstRow; y < lastRow; y++)
    {
        // Reset for this row
        int32_t mapX       = iMapX;
        int32_t texOffsetX = iTexOffsetX;
        int32_t copySize   = iCopySize;

        // For the entire row, in CELL_SIZE steps
        for (int32_t x = 0; x < TFT_WIDTH; /* x updated in the loop */)
        {
            // Get this cell type and the texture for it
            rayMapCellType_t type = ray->map.tiles[mapX][mapY].type;
            const wsg_t* texture  = getTexByType(ray, type);

            // Copy one row from the texture to the framebuffer
            memcpy(fb, &texture->px[CELL_SIZE * texOffsetY + texOffsetX], copySize);

            // Advance the framebuffer
            fb += copySize;

            // Advance the row pixel
            x += copySize;

            // Set the texture offset and copy size for the next
            texOffsetX = 0;
            copySize   = CELL_SIZE;

            // Make sure it doesn't go out of bounds
            if (x + copySize > TFT_WIDTH)
            {
                copySize = TFT_WIDTH - x;
            }

            // Iterate cell
            mapX++;
        }

        // Iterate the texture offset for each row
        texOffsetY++;
        if (texOffsetY >= CELL_SIZE)
        {
            // Advance to the next cell
            mapY++;
            texOffsetY = 0;
        }
    }
}

void drawCommonList(ray_t* ray, list_t* list, int camX, int camY, paletteColor_t bbColor)
{
    node_t* node = list->first;
    while (node)
    {
        rayObjCommon_t* obj = node->val;
        drawWsgSimple(obj->sprite,                                    //
                      TO_PX(obj->posX) - camX - (obj->sprite->w / 2), //
                      TO_PX(obj->posY) - camY - (obj->sprite->h / 2));

        if (cTransparent != bbColor)
        {
            if (obj->bound.box.h)
            {
                rectangle_t bb = rayGetObjBB(obj);
                drawRect(TO_PX(bb.pos.x) - camX,            //
                         TO_PX(bb.pos.y) - camY,            //
                         TO_PX(bb.pos.x + bb.width) - camX, //
                         TO_PX(bb.pos.y + bb.height) - camY, bbColor);
            }
            else
            {
                drawCircle(TO_PX(obj->posX), TO_PX(obj->posY), obj->bound.radius, bbColor);
            }
        }
        node = node->next;
    }
}

void drawForeground2d(ray_t* ray)
{
    int32_t camX = ray->camera.x;
    int32_t camY = ray->camera.y;

    drawCommonList(ray, &ray->scenery, camX, camY, cTransparent);
    drawCommonList(ray, &ray->items, camX, camY, cTransparent);
    drawCommonList(ray, &ray->enemies, camX, camY, cTransparent);

    for (int bIdx = 0; bIdx < MAX_RAY_BULLETS; bIdx++)
    {
        rayObjCommon_t* obj = &ray->bullets[bIdx].c;
        if (obj->type & BULLET && obj->id >= 0)
        {
            // Boomerang rotates, otherwise point the sprite in the direction it's traveling
            int32_t angle = (OBJ_BULLET_BOOMERANG == obj->type)
                                ? ray->bullets[bIdx].c.spriteRotation
                                : rayGetEightWayAngle(ray->bullets[bIdx].velX, ray->bullets[bIdx].velY);
            drawWsg(obj->sprite,                                    //
                    TO_PX(obj->posX) - camX - (obj->sprite->w / 2), //
                    TO_PX(obj->posY) - camY - (obj->sprite->h / 2), false, false, angle);

            // Draw a filled circle for a bomb explosions
            if (OBJ_BULLET_BOMB == obj->type)
            {
                if (ray->bullets[bIdx].fuseUs > 0 && ray->bullets[bIdx].c.bound.radius > 0)
                {
                    drawCircleFilled(TO_PX(obj->posX) - camX, TO_PX(obj->posY) - camY, TO_PX(obj->bound.radius), c530);
                }
            }

            if (obj->bound.box.h)
            {
                rectangle_t bb = rayGetObjBB(obj);
                drawRect(TO_PX(bb.pos.x) - camX,            //
                         TO_PX(bb.pos.y) - camY,            //
                         TO_PX(bb.pos.x + bb.width) - camX, //
                         TO_PX(bb.pos.y + bb.height) - camY, c505);
            }
            else
            {
                drawCircle(TO_PX(obj->posX) - camX, TO_PX(obj->posY) - camY, obj->bound.radius, c505);
            }
        }
    }

    int16_t pSpriteX = TO_PX(ray->p.posX) - camX - (ray->ps.sprite->w / 2);
    int16_t pSpriteY = TO_PX(ray->p.posY) - camY - (ray->ps.sprite->h / 2);
    if (ray->ps.jumpPos || ray->ps.jumpVel)
    {
        int16_t spriteRadius = (ray->ps.sprite->w / 2);
        drawEllipseFilled(pSpriteX + spriteRadius, pSpriteY + ray->ps.sprite->h, spriteRadius, spriteRadius / 2, c111);
    }

    drawWsg(ray->ps.sprite, pSpriteX, pSpriteY + TO_PX(ray->ps.jumpPos), false, false,
            rayGetEightWayAngle(ray->p.dirX, ray->p.dirY));

    if (ray->ps.swordTimerUs > 0)
    {
        line_t sword = rayGetSwordLineSegment(ray);
        drawLineFast(TO_PX(sword.p1.x) - camX, //
                     TO_PX(sword.p1.y) - camY, //
                     TO_PX(sword.p2.x) - camX, //
                     TO_PX(sword.p2.y) - camY, c550);
    }

    if (ray->ps.shieldTimerUs > 0)
    {
        static const paletteColor_t zColors[] = {
            c500,
            c150,
            c045,
            c305,
        };
        drawCircle(TO_PX(ray->p.posX), TO_PX(ray->p.posY + ray->ps.jumpPos), CELL_SIZE / 2,
                   zColors[ray->ps.shieldZone]);
    }

    // Draw HUD
#define X_MARGIN 24
    int16_t xOff = X_MARGIN;

    // Draw hearts
    wsg_t* heart      = getTexByType(ray, OBJ_ITEM_HEART);
    int16_t yHeartOff = (CELL_SIZE - heart->h) / 2;
    for (int i = 0; i < ray->p.health; i++)
    {
        drawWsgSimple(heart, xOff, yHeartOff);
        xOff += heart->w + 2;
    }

    // Measure MPoints
    wsg_t* mpoint        = getTexByType(ray, OBJ_ITEM_MPOINT_1);
    char mpointCount[32] = {0};
    sprintf(mpointCount, "%" PRIu32, ray->p.mpoints);
    int16_t mPointWidth = mpoint->w + 2 + textWidth(&ray->ibm, mpointCount);
    xOff                = TFT_WIDTH - mPointWidth - X_MARGIN;

    // Draw MPoints
    drawWsgSimple(mpoint, xOff, 0);
    xOff += mpoint->w + 2;
    drawText(&ray->ibm, c555, mpointCount, xOff, (mpoint->h - ray->ibm.height) / 2);
}
