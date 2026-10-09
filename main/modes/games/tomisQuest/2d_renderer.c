//==============================================================================
// Includes
//==============================================================================

#include "fp_math.h"
#include "2d_renderer.h"
#include "ray_tex_manager.h"
#include "ray_player.h"
#include "ray_enemy.h"

//==============================================================================
// Defines
//==============================================================================

/**
 * @brief Convert fixed point units to pixel units
 */
#define TO_PX(x) ((CELL_SIZE * (x)) / 256)

//==============================================================================
// Function Declarations
//==============================================================================

static void drawCommonList(list_t* list, int camX, int camY, paletteColor_t bbColor);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Callback function to draw the 2D background
 *
 * @param ray The entire game state
 * @param firstRow The first row of pixels to draw
 * @param lastRow The last row of pixels to draw
 */
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

    // A cache for WSGs used on this row
    const wsg_t* tileRow[(TFT_WIDTH / CELL_SIZE) + 1] = {NULL};

    // For each pixel row in this update
    for (uint32_t y = firstRow; y < lastRow; y++)
    {
        // Reset for this row
        int32_t mapX       = iMapX;
        int32_t texOffsetX = iTexOffsetX;
        int32_t copySize   = iCopySize;

        // For the entire row, in CELL_SIZE steps
        for (int32_t x = 0, tIdx = 0; x < TFT_WIDTH; tIdx++ /* x updated in the loop */)
        {
            // If there is no cached texture, get one
            if (!tileRow[tIdx])
            {
                tileRow[tIdx] = ray->typeToTexMap[ray->map.tiles[mapX][mapY].type];
            }

            // Copy one row from the texture to the framebuffer
            memcpy(fb, &tileRow[tIdx]->px[CELL_SIZE * texOffsetY + texOffsetX], copySize);

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
            // Invalidate the cache
            memset(tileRow, 0, sizeof(tileRow));
        }
    }
}

/**
 * @brief Draw a list of common objects
 *
 * @param list The list of rayObjCommon_t to draw
 * @param camX The X camera position
 * @param camY The Y camera position
 * @param bbColor A color to draw the bounding box, may be cTransparent
 */
static void drawCommonList(list_t* list, int camX, int camY, paletteColor_t bbColor)
{
    node_t* node = list->first;
    while (node)
    {
        rayObjCommon_t* obj = node->val;
        drawWsgSolid(obj->sprite,                                    //
                     TO_PX(obj->posX) - camX - (obj->sprite->w / 2), //
                     TO_PX(obj->posY) - camY - (obj->sprite->h / 2), obj->spriteMirrored, false, obj->spriteRotation,
                     obj->solidColor);

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
                drawCircle(TO_PX(obj->posX), TO_PX(obj->posY), TO_PX(obj->bound.radius), bbColor);
            }
        }
        node = node->next;
    }
}

/**
 * @brief Smoothly move the 2D camera to the target
 *
 * @param ray The entire game state
 * @param elapsedUs The time elapsed since last call
 */
void update2dCamera(ray_t* ray, uint32_t elapsedUs)
{
    // Run timers for camera movement
    RUN_TIMER_EVERY(ray->cameraTimer, (1000000 / TFT_WIDTH), elapsedUs, {
        if (ray->camera.x < ray->p.cameraTarget.x)
        {
            ray->camera.x++;
        }
        else if (ray->camera.x > ray->p.cameraTarget.x)
        {
            ray->camera.x--;
        }

        if (ray->camera.y < ray->p.cameraTarget.y)
        {
            ray->camera.y++;
        }
        else if (ray->camera.y > ray->p.cameraTarget.y)
        {
            ray->camera.y--;
        }
    });
}

/**
 * @brief Draw the game foreground (scenery, sprites, etc.)
 *
 * @param ray The entire game state
 */
void drawForeground2d(ray_t* ray)
{
    int32_t camX = ray->camera.x;
    int32_t camY = ray->camera.y;

    drawCommonList(&ray->scenery, camX, camY, cTransparent);
    drawCommonList(&ray->items, camX, camY, cTransparent);
    drawCommonList(&ray->enemies, camX, camY, cTransparent);

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

            // if (obj->bound.box.h)
            // {
            //     rectangle_t bb = rayGetObjBB(obj);
            //     drawRect(TO_PX(bb.pos.x) - camX,            //
            //              TO_PX(bb.pos.y) - camY,            //
            //              TO_PX(bb.pos.x + bb.width) - camX, //
            //              TO_PX(bb.pos.y + bb.height) - camY, c505);
            // }
            // else
            // {
            //     drawCircle(TO_PX(obj->posX) - camX, TO_PX(obj->posY) - camY, TO_PX(obj->bound.radius), c505);
            // }
        }
    }

    int16_t pSpriteX = TO_PX(ray->p.posX) - camX - (ray->ps.sprite->w / 2);
    int16_t pSpriteY = TO_PX(ray->p.posY) - camY - (ray->ps.sprite->h / 2);
    if (ray->ps.jumpPos || ray->ps.jumpVel)
    {
        int16_t spriteRadius = (ray->ps.sprite->w / 2);
        drawEllipseFilled(pSpriteX + spriteRadius, pSpriteY + ray->ps.sprite->h, spriteRadius, spriteRadius / 2, c111);
    }

    paletteColor_t playerSolidColor = cTransparent;
    if (ray->ps.iFrameTimer > 0)
    {
        playerSolidColor = ((6 * ray->ps.iFrameTimer) / ENEMY_DEFAULT_IFRAMES_US) & 0x01 ? c111 : c444;
    }

    drawWsgSolid(ray->ps.sprite, pSpriteX, pSpriteY + TO_PX(ray->ps.jumpPos), false, false,
                 rayGetEightWayAngle(ray->p.dirX, ray->p.dirY), playerSolidColor);
    // rectangle_t bb = rayGetPlayerBB(ray);
    // drawRect(TO_PX(bb.pos.x) - camX,            //
    //          TO_PX(bb.pos.y) - camY,            //
    //          TO_PX(bb.pos.x + bb.width) - camX, //
    //          TO_PX(bb.pos.y + bb.height) - camY, c050);

    if (ray->ps.swordTimerUs > 0)
    {
        line_t sword = rayGetSwordLineSegment(ray);
        drawLineFast(TO_PX(sword.p1.x) - camX,                   //
                     TO_PX(sword.p1.y + ray->ps.jumpPos) - camY, //
                     TO_PX(sword.p2.x) - camX,                   //
                     TO_PX(sword.p2.y + ray->ps.jumpPos) - camY, c005);
    }

    if (ray->ps.shieldTimerUs > 0)
    {
        static const paletteColor_t zColors[] = {
            c500,
            c150,
            c045,
            c305,
        };
        drawCircleOutline(TO_PX(ray->p.posX) - camX,                   // Center
                          TO_PX(ray->p.posY + ray->ps.jumpPos) - camY, // Center
                          CELL_SIZE / 2,                               // Radius
                          3,                                           // Stroke
                          zColors[ray->ps.shieldZone]);                // Color
    }

    // Draw HUD
#define X_MARGIN  24
#define X_SPACING 2

    int16_t xOff = X_MARGIN;

    // Draw hearts
    wsg_t* heart      = getTexByType(ray, OBJ_ITEM_HEART);
    int16_t yHeartOff = (CELL_SIZE - heart->h) / 2;
    for (int i = 0; i < ray->p.health; i++)
    {
        drawWsgSimple(heart, xOff, yHeartOff);
        xOff += heart->w + X_SPACING;
    }

    // Start drawing here, right to left
    xOff = TFT_WIDTH - X_MARGIN;

    // Mpoint icon
    wsg_t* mpoint = getTexByType(ray, OBJ_ITEM_MPOINT_1);

    // Mpoint Text
    char mpointCount[32] = {0};
    sprintf(mpointCount, "%" PRIu32, ray->p.mpoints);
    xOff -= textWidth(&ray->ibm, mpointCount);
    drawText(&ray->ibm, c555, mpointCount, xOff, (mpoint->h - ray->ibm.height) / 2);

    // Mpoint Icon
    xOff -= (X_SPACING + mpoint->w);
    drawWsgSimple(mpoint, xOff, 0);

    // Draw Keys
    if (MS_DUNGEON == getRayMapMetadata(ray->p.mapId)->style)
    {
        // List of held keys
        const bool* keys[] = {
            &ray->ps.keyMask.k_key,
            &ray->ps.keyMask.b_key,
            &ray->ps.keyMask.g_key,
            &ray->ps.keyMask.r_key,
        };

        // Associated icons, in order
        const wsg_t* wsgs[] = {
            getTexByType(ray, OBJ_ITEM_K_KEY),
            getTexByType(ray, OBJ_ITEM_B_KEY),
            getTexByType(ray, OBJ_ITEM_G_KEY),
            getTexByType(ray, OBJ_ITEM_R_KEY),
        };

        // For oeach key
        for (int kIdx = 0; kIdx < ARRAY_SIZE(keys); kIdx++)
        {
            // If it is held
            if (*keys[kIdx])
            {
                // Draw the icon
                xOff -= (X_SPACING + wsgs[kIdx]->w);
                drawWsgSimple(wsgs[kIdx], xOff, 0);
            }
        }
    }

    // DRAW_FPS_COUNTER(ray->ibm);
}

/**
 * @brief Center the camera on the player, keeping it in world bounds
 *
 * @param ray The entire game state
 */
void rayCenterCameraOnPlayer(ray_t* ray)
{
    // Center camera on player
    ray->camera.x = TO_PX(ray->p.posX) - (TFT_WIDTH / 2) + (ray->ps.sprite->w / 2);
    ray->camera.y = TO_PX(ray->p.posY) - (TFT_HEIGHT / 2) + (ray->ps.sprite->h / 2);

    // Clamp camera to world bounds
    ray->camera.x = CLAMP(ray->camera.x, 0, (ray->map.w * CELL_SIZE) - TFT_WIDTH);
    ray->camera.y = CLAMP(ray->camera.y, 0, (ray->map.h * CELL_SIZE) - TFT_HEIGHT);

    // Set target to camera
    ray->p.cameraTarget.x = ray->camera.x;
    ray->p.cameraTarget.y = ray->camera.y;
}
