//==============================================================================
// Includes
//==============================================================================

#include "mode_ray.h"
#include "ray_renderer.h"
#include "fp_math.h"
#include "hdw-tft.h"

//==============================================================================
// Defines
//==============================================================================

#define TEX_WIDTH  64
#define TEX_HEIGHT 64

#define EX_CEIL_PRECISION_BITS 8
#define EX_CEIL_PRECISION      (1 << EX_CEIL_PRECISION_BITS)

//==============================================================================
// Function Prototypes
//==============================================================================

static int objDistComparator(const void* obj1, const void* obj2);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO
 *
 * @param ray
 * @param firstRow
 * @param lastRow
 */
void castFloorCeiling(ray_t* ray, int16_t firstRow, int16_t lastRow)
{
    // We'll be drawing pixels, so set this up
    SETUP_FOR_TURBO();

    // Loop through each horizontal row
    for (int16_t y = firstRow; y < lastRow; y++)
    {
        bool isFloor = y > TFT_HEIGHT / 2;

        // rayDir for leftmost ray (x = 0) and rightmost ray (x = w)
        q24_8 rayDirX0 = SUB_FX(ray->dirX, ray->planeX);
        q24_8 rayDirY0 = SUB_FX(ray->dirY, ray->planeY);
        q24_8 rayDirX1 = ADD_FX(ray->dirX, ray->planeX);
        q24_8 rayDirY1 = ADD_FX(ray->dirY, ray->planeY);

        // Current y position compared to the center of the screen (the horizon)
        int16_t p;
        if (isFloor)
        {
            p = (y - (TFT_HEIGHT / 2));
        }
        else
        {
            p = ((TFT_HEIGHT / 2) - y);
        }

        // Don't divide by zero (infinite distance on the horizon)
        if (0 == p)
        {
            continue;
        }

        // Vertical position of the camera.
        // NOTE: with 0.5, it's exactly in the center between floor and ceiling,
        // matching also how the walls are being casted. For different values
        // than 0.5, a separate loop must be done for ceiling and floor since
        // they're no longer symmetrical.
        q24_8 camZ;
        if (isFloor)
        {
            camZ = ADD_FX(TO_FX(TFT_HEIGHT / 2), ray->posZ);
        }
        else
        {
            camZ = SUB_FX(TO_FX(TFT_HEIGHT / 2), ray->posZ);
        }

        // Horizontal distance from the camera to the floor for the current row.
        // 0.5 is the z position exactly in the middle between floor and ceiling.
        // NOTE: this is affine texture mapping, which is not perspective correct
        // except for perfectly horizontal and vertical surfaces like the floor.
        // NOTE: this formula is explained as follows: The camera ray goes through
        // the following two points: the camera itself, which is at a certain
        // height (posZ), and a point in front of the camera (through an imagined
        // vertical plane containing the screen pixels) with horizontal distance
        // 1 from the camera, and vertical position p lower than posZ (posZ - p). When going
        // through that point, the line has vertically traveled by p units and
        // horizontally by 1 unit. To hit the floor, it instead needs to travel by
        // posZ units. It will travel the same ratio horizontally. The ratio was
        // 1 / p for going through the camera plane, so to go posZ times farther
        // to reach the floor, we get that the total horizontal distance is posZ / p.
        q24_8 rowDistance = camZ / p;

        // real world coordinates of the leftmost column. This will be updated as we step to the right.
        q16_16 floorX = ADD_FX(ray->posX, MUL_FX(rowDistance, rayDirX0)) * EX_CEIL_PRECISION;
        q16_16 floorY = ADD_FX(ray->posY, MUL_FX(rowDistance, rayDirY0)) * EX_CEIL_PRECISION;

        // calculate the real world step vector we have to add for each x (parallel to camera plane)
        // adding step by step avoids multiplications with a weight in the inner loop
        q16_16 floorStepX = (MUL_FX(rowDistance, SUB_FX(rayDirX1, rayDirX0)) * EX_CEIL_PRECISION) / TFT_WIDTH;
        q16_16 floorStepY = (MUL_FX(rowDistance, SUB_FX(rayDirY1, rayDirY0)) * EX_CEIL_PRECISION) / TFT_WIDTH;

        // Loop through each pixel
        for (int16_t x = 0; x < TFT_WIDTH; ++x)
        {
            // the cell coord is simply got from the integer parts of floorX and floorY
            // int cellX = FROM_FX(floorX);
            // int cellY = FROM_FX(floorY);

            // get the texture coordinate from the fractional part
            q16_16 fracPartX = floorX - (floorX & ~((1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS)) - 1));
            q16_16 fracPartY = floorY - (floorY & ~((1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS)) - 1));
            uint16_t tx      = ((TEX_WIDTH * fracPartX) / (1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS))) % TEX_WIDTH;
            uint16_t ty      = ((TEX_HEIGHT * fracPartY) / (1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS))) % TEX_HEIGHT;

            floorX += floorStepX;
            floorY += floorStepY;

            // Draw the pixel
            if (isFloor)
            {
                TURBO_SET_PIXEL(x, y, ray->texFloor.px[TEX_WIDTH * ty + tx]);
            }
            else
            {
                TURBO_SET_PIXEL(x, y, ray->texCeiling.px[TEX_WIDTH * ty + tx]);
            }
        }
    }
}

/**
 * @brief TODO
 *
 * @param ray
 */
void castWalls(ray_t* ray)
{
    // We'll be drawing pixels, so set this up
    SETUP_FOR_TURBO();

    // Compute cartesian direction and camera plane from direction
    // direction vector, cartesian coordinates
    ray->dirX = -getCos1024(FROM_FX(ray->dirAngle)) / 4; // trig functions are already << 10, so >> 2 to get to << 8
    ray->dirY = getSin1024(FROM_FX(ray->dirAngle)) / 4;

    // the 2d rayCaster version of camera plane, orthogonal to the direction vector and scaled to 2/3
    ray->planeX = MUL_FX(((1 << FRAC_BITS) * 2) / 3, ray->dirY);
    ray->planeY = MUL_FX(-((1 << FRAC_BITS) * 2) / 3, ray->dirX);

    // For each ray
    for (int16_t x = 0; x < TFT_WIDTH; x++)
    {
        // calculate ray position and direction
        q24_8 cameraX = ((x * (2 << FRAC_BITS)) / TFT_WIDTH) - (1 << FRAC_BITS); // x-coordinate in camera space
        q24_8 rayDirX = ADD_FX(ray->dirX, MUL_FX(ray->planeX, cameraX));
        q24_8 rayDirY = ADD_FX(ray->dirY, MUL_FX(ray->planeY, cameraX));

        // which box of the map we're in
        int16_t mapX = FROM_FX(ray->posX);
        int16_t mapY = FROM_FX(ray->posY);

        // length of ray from one x or y-side to next x or y-side
        // these are derived as:
        // deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX))
        // deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY))
        // which can be simplified to abs(|rayDir| / rayDirX) and abs(|rayDir| / rayDirY)
        // where |rayDir| is the length of the vector (rayDirX, rayDirY). Its length,
        // unlike (ray->dirX, ray->dirY) is not 1, however this does not matter, only the
        // ratio between deltaDistX and deltaDistY matters, due to the way the DDA
        // stepping further below works. So the values can be computed as below.
        // Division through zero is prevented
        q24_8 deltaDistX = (rayDirX == 0) ? INT32_MAX : ABS(DIV_FX((1 << FRAC_BITS), rayDirX));
        q24_8 deltaDistY = (rayDirY == 0) ? INT32_MAX : ABS(DIV_FX((1 << FRAC_BITS), rayDirY));

        // what direction to step in x or y-direction (either +1 or -1)
        int8_t stepX = 0;
        int8_t stepY = 0;

        // length of ray from current position to next x or y-side
        q24_8 sideDistX = 0;
        q24_8 sideDistY = 0;

        // calculate step and initial sideDist (x)
        if (rayDirX < 0)
        {
            stepX     = -1;
            sideDistX = MUL_FX(SUB_FX(ray->posX, TO_FX(mapX)), deltaDistX);
        }
        else if (rayDirX > 0)
        {
            stepX     = 1;
            sideDistX = MUL_FX(SUB_FX(TO_FX(mapX + 1), ray->posX), deltaDistX);
        }

        // calculate step and initial sideDist (y)
        if (rayDirY < 0)
        {
            stepY     = -1;
            sideDistY = MUL_FX(SUB_FX(ray->posY, TO_FX(mapY)), deltaDistY);
        }
        else if (rayDirY > 0)
        {
            stepY     = 1;
            sideDistY = MUL_FX(SUB_FX(TO_FX(mapY + 1), ray->posY), deltaDistY);
        }

        bool hit  = false; // was there a wall hit?
        bool side = false; // was a NS or a EW wall hit?

        q24_8 wallX;                            // where exactly the wall was hit
        int16_t lineHeight, drawStart, drawEnd; // the height of the wall strip
        q24_8 perpWallDist;
        // perform DDA
        while (false == hit)
        {
            // jump to next map square, either in x-direction, or in y-direction
            if (sideDistX < sideDistY)
            {
                sideDistX = ADD_FX(sideDistX, deltaDistX);
                mapX += stepX;
                side = false;
            }
            else
            {
                sideDistY = ADD_FX(sideDistY, deltaDistY);
                mapY += stepY;
                side = true;
            }

            // Check if ray has hit a wall or door
            switch (ray->map.tiles[mapX][mapY])
            {
                case BG_WALL:
                case BG_DOOR:
                {
                    // Calculate distance projected on camera direction. This is the shortest distance from the point
                    // where the wall is hit to the camera plane. Euclidean to center camera point would give fisheye
                    // effect! This can be computed as (mapX - ray->posX + (1 - stepX) / 2) / rayDirX for side == 0, or
                    // same formula with Y for size == 1, but can be simplified to the code below thanks to how sideDist
                    // and deltaDist are computed: because they were left scaled to |rayDir|. sideDist is the entire
                    // length of the ray above after the multiple steps, but we subtract deltaDist once because one step
                    // more into the wall was taken above.
                    if (false == side)
                    {
                        perpWallDist = SUB_FX(sideDistX, deltaDistX);
                    }
                    else
                    {
                        perpWallDist = SUB_FX(sideDistY, deltaDistY);
                    }

                    // Save the distance to this wall strip, used for sprite casting
                    ray->wallDistBuffer[x] = perpWallDist;

                    // Calculate height of line to draw on screen, make sure not to div by zero
                    lineHeight = (0 == perpWallDist) ? (TFT_HEIGHT) : (TO_FX(TFT_HEIGHT) / perpWallDist);

                    // calculate lowest and highest pixel to fill in current stripe
                    drawStart = (TFT_HEIGHT - lineHeight) / 2 + (ray->posZ / perpWallDist);
                    drawEnd   = (TFT_HEIGHT + lineHeight) / 2 + (ray->posZ / perpWallDist);

                    // TODO sometimes textures wraparound b/c the math for wallX comes out to be like
                    // 19.003 -> 0
                    // 19.000 -> 0
                    // 18.995 -> 63

                    // calculate value of wallX
                    if (side == 0)
                    {
                        wallX = ADD_FX(ray->posY, MUL_FX(perpWallDist, rayDirY));
                    }
                    else
                    {
                        wallX = ADD_FX(ray->posX, MUL_FX(perpWallDist, rayDirX));
                    }
                    wallX = SUB_FX(wallX, FLOOR_FX(wallX));

                    // For sliding doors, only collide with the closed part
                    if (BG_DOOR == ray->map.tiles[mapX][mapY])
                    {
                        // If the fraction of the door the ray hits is closed
                        if (wallX >= ray->doorOpen)
                        {
                            // Count it as a hit
                            hit = true;
                            // Adjust wallX to start drawing the texture at the door's edge rather than the map cell's
                            // edge
                            wallX -= ray->doorOpen;
                        }
                    }
                    else
                    {
                        // Wall was hit
                        hit = true;
                    }
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
                case OBJ_DELETE:
                default:
                {
                    // Hit some type we don't care about
                    break;
                }
            }
        }

        // x coordinate on the texture
        int8_t texX = FROM_FX(wallX * TEX_WIDTH);

        // Mirror X texture coordinate for certain walls
        if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0))
        {
            texX = TEX_WIDTH - texX - 1;
        }

        // How much to increase the texture coordinate per screen pixel
        q8_24 step = (TEX_HEIGHT << 24) / lineHeight;

        // Starting texture coordinate. If it would start offscreen, start it at the right spot onscreen instead
        q8_24 texPos = 0;
        if (drawStart < 0)
        {
            // Start the texture somewhere in the middle
            texPos = (-drawStart) * step;
            // Always start drawing on screen
            drawStart = 0;
        }

        // Also make sure to not draw off the bottom of the display
        if (drawEnd > TFT_HEIGHT)
        {
            drawEnd = TFT_HEIGHT;
        }

        // Pick the texture based on the map tile
        paletteColor_t* tex;
        if (BG_DOOR == ray->map.tiles[mapX][mapY])
        {
            tex = ray->texDoor.px;
        }
        else
        {
            tex = ray->texWall.px;
        }

        // Draw a vertical strip
        for (int16_t y = drawStart; y < drawEnd; y++)
        {
            // Cast the texture coordinate to integer, and mod it to ensure no out of bounds reads
            int8_t texY = (texPos >> 24) % TEX_HEIGHT;

            // Increment the texture position for the next iteration
            texPos += step;

            // Get the color from the texture
            TURBO_SET_PIXEL(x, y, tex[TEX_HEIGHT * texY + texX]);
        }
    }
}

/**
 * @brief Compare two rayObj_t* based on distance
 *
 * @param obj1 A rayObj_t* to compare
 * @param obj2 Another rayObj_t* to compare
 * @return an integer less than, equal to, or greater than zero if the first argument is considered to be respectively
 * less than, equal to, or greater than the second.
 */
static int objDistComparator(const void* obj1, const void* obj2)
{
    return (((const rayObj_t*)obj1)->dist - ((const rayObj_t*)obj2)->dist);
}

/**
 * @brief TODO
 *
 * @param ray
 */
void castSprites(ray_t* ray)
{
    // Setup to draw
    SETUP_FOR_TURBO();

    // Assign each sprite a distance from the player
    for (int i = 0; i < MAX_RAY_OBJS; i++)
    {
        // Make a convenience pointer
        rayObj_t* obj = &ray->objs[i];
        if (-1 != obj->id)
        {
            // sqrt not taken, unneeded
            obj->dist = ((ray->posX - obj->posX) * (ray->posX - obj->posX)) + //
                        ((ray->posY - obj->posY) * (ray->posY - obj->posY));
        }
    }
    // Sort the sprites by distance
    qsort(ray->objs, MAX_RAY_OBJS, sizeof(rayObj_t), objDistComparator);

    // after sorting the sprites, do the projection and draw them
    for (int i = 0; i < MAX_RAY_OBJS; i++)
    {
        // Make a convenience pointer
        rayObj_t* obj = &ray->objs[i];

        // Make sure this object slot is occupied
        if (-1 != obj->id)
        {
            // translate sprite position to relative to camera
            q24_8 spriteX = SUB_FX(obj->posX, ray->posX);
            q24_8 spriteY = SUB_FX(obj->posY, ray->posY);

            // transform sprite with the inverse camera matrix
            //  [ planeX   dirX ] -1                                       [ dirY      -dirX ]
            //  [               ]       =  1/(planeX*dirY-dirX*planeY) *   [                 ]
            //  [ planeY   dirY ]                                          [ -planeY  planeX ]

            // required for correct matrix multiplication
            q24_8 invDetDivisor = SUB_FX(MUL_FX(ray->planeX, ray->dirY), MUL_FX(ray->dirX, ray->planeY));

            // this is actually the depth inside the screen, that what Z is in 3D
            q24_8 transformY
                = DIV_FX(ADD_FX(MUL_FX(-ray->planeY, spriteX), MUL_FX(ray->planeX, spriteY)), invDetDivisor);

            // If this is negative, the texture isn't going to be drawn, so just stop here
            if (transformY <= 0)
            {
                // Not drawn in bounds, so continue to the next object
                continue;
            }

            // TODO do all the X math first to see if its on screen, then do Y math??
            q24_8 transformX = DIV_FX(SUB_FX(MUL_FX(ray->dirY, spriteX), MUL_FX(ray->dirX, spriteY)), invDetDivisor);

            // The division here takes the number from q24_8 to int16_t
            int16_t spriteScreenX = (TFT_WIDTH * (transformX + transformY)) / (2 * transformY);

            // calculate width of the sprite
            // TODO is this getting height???
            int16_t spriteWidth = TO_FX(TFT_HEIGHT) / transformY;
            spriteWidth         = ABS(spriteWidth);
            int16_t drawStartX  = spriteScreenX - (spriteWidth / 2);
            if (drawStartX < 0)
            {
                drawStartX = 0;
            }
            int16_t drawEndX = spriteScreenX + (spriteWidth / 2);
            if (drawEndX >= TFT_WIDTH)
            {
                drawEndX = TFT_WIDTH - 1;
            }

            if (drawStartX >= TFT_WIDTH || drawEndX < 0)
            {
                // Not drawn in bounds, so continue to the next object
                continue;
            }

            // calculate height of the sprite on screen
            // using 'transformY' instead of the real distance prevents fisheye
            int16_t spriteHeight = TO_FX(TFT_HEIGHT) / transformY;
            spriteHeight         = ABS(spriteHeight);
            // calculate lowest and highest pixel to fill in current stripe
            int16_t drawStartY = (-spriteHeight + TFT_HEIGHT) / 2;
            if (drawStartY < 0)
            {
                drawStartY = 0;
            }
            int16_t drawEndY = (spriteHeight + TFT_HEIGHT) / 2;
            if (drawEndY >= TFT_HEIGHT)
            {
                drawEndY = TFT_HEIGHT - 1;
            }

            // Get initial texture X coordinate and the step per-screen-pixel
            q16_16 texX      = ((TEX_WIDTH * (drawStartX - spriteScreenX + (spriteWidth / 2))) << 16) / (spriteWidth);
            q16_16 texXDelta = (TEX_WIDTH << 16) / spriteWidth;

            // loop through every vertical stripe of the sprite on screen
            for (int16_t stripe = drawStartX; stripe < drawEndX; stripe++)
            {
                // Check wallDistBuffer to make sure the sprite is on the screen
                if (transformY < ray->wallDistBuffer[stripe])
                {
                    // Get initial texture Y coordinate and the step per-screen-pixel
                    q16_16 texY
                        = ((TEX_HEIGHT * (drawStartY + ((spriteHeight - TFT_HEIGHT) / 2))) << 16) / (spriteHeight);
                    q16_16 texYDelta = (TEX_HEIGHT << 16) / spriteHeight;

                    // for every pixel of the current stripe
                    for (int16_t y = drawStartY; y < drawEndY; y++)
                    {
                        // get current color from the texture, draw if not transparent
                        paletteColor_t color = ray->texPirate.px[TEX_WIDTH * (texY >> 16) + (texX >> 16)];
                        if (cTransparent != color)
                        {
                            TURBO_SET_PIXEL(stripe, y, color);
                        }
                        texY += texYDelta;
                    }
                }
                texX += texXDelta;
            }
        }
    }
}
