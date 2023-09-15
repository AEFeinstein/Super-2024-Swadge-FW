//==============================================================================
// Includes
//==============================================================================

#include "mode_ray.h"
#include "ray_renderer.h"
#include "ray_tex_manager.h"
#include "fp_math.h"
#include "hdw-tft.h"

//==============================================================================
// Defines
//==============================================================================

// When casting floor and ceiling, use 8 more decimal bits and 8 fewer whole number bits
#define EX_CEIL_PRECISION_BITS 8

// Half width of the lock zone when locking on enemies
#define LOCK_ZONE 16

//==============================================================================
// Structs
//==============================================================================

/// Helper struct to sort rayObjCommon_t by distance from the player
typedef struct
{
    rayObjCommon_t* obj;
    uint32_t dist;
} objDist_t;

//==============================================================================
// Const data
//==============================================================================

// LUT for a palette swap when in X-Ray loadout mode
const paletteColor_t xrayPaletteSwap[]
    = {c555, c554, c553, c552, c551, c550, c545, c544, c543, c542, c541, c540, c535,        c534, c533, c532, c531,
       c530, c525, c524, c523, c522, c521, c520, c515, c514, c513, c512, c511, c510,        c505, c504, c503, c502,
       c501, c500, c455, c454, c453, c452, c451, c450, c445, c444, c443, c442, c441,        c440, c435, c434, c433,
       c432, c431, c430, c425, c424, c423, c422, c421, c420, c415, c414, c413, c412,        c411, c410, c405, c404,
       c403, c402, c401, c400, c355, c354, c353, c352, c351, c350, c345, c344, c343,        c342, c341, c340, c335,
       c334, c333, c332, c331, c330, c325, c324, c323, c322, c321, c320, c315, c314,        c313, c312, c311, c310,
       c305, c304, c303, c302, c301, c300, c255, c254, c253, c252, c251, c250, c245,        c244, c243, c242, c241,
       c240, c235, c234, c233, c232, c231, c230, c225, c224, c223, c222, c221, c220,        c215, c214, c213, c212,
       c211, c210, c205, c204, c203, c202, c201, c200, c155, c154, c153, c152, c151,        c150, c145, c144, c143,
       c142, c141, c140, c135, c134, c133, c132, c131, c130, c125, c124, c123, c122,        c121, c120, c115, c114,
       c113, c112, c111, c110, c105, c104, c103, c102, c101, c100, c055, c054, c053,        c052, c051, c050, c045,
       c044, c043, c042, c041, c040, c035, c034, c033, c032, c031, c030, c025, c024,        c023, c022, c021, c020,
       c015, c014, c013, c012, c011, c010, c005, c004, c003, c002, c001, c000, cTransparent};

//==============================================================================
// Function Prototypes
//==============================================================================

static int objDistComparator(const void* obj1, const void* obj2);
static bool rayIntersectsDoor(bool side, int32_t mapX, int32_t mapY, q24_8 posX, q24_8 posY, q24_8 rayDirX,
                              q24_8 rayDirY, q24_8 deltaDistX, q24_8 deltaDistY, q24_8 doorOpen);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draw a section of the floor and ceiling pixels. The floor and ceiling are drawn first. This iterates over the
 * screen top-to-bottom and draws horizontal rows.
 * This is called from the background draw callback
 *
 * @param ray The entire game state
 * @param firstRow The first row to draw
 * @param lastRow The last row to draw
 */
void castFloorCeiling(ray_t* ray, int32_t firstRow, int32_t lastRow)
{
    // We'll be drawing pixels, so set this up
    SETUP_FOR_TURBO();

    // Boolean if the colors should be drawn inverted
    bool isXray = (LO_XRAY == ray->loadout);

    // Track which cell the ceiling or floor is being drawn in
    uint32_t cellX = 0;
    uint32_t cellY = 0;

    // Track the last cell the ceiling or floor was drawn in to reset textures which it changes
    uint32_t lastCellX = -1;
    uint32_t lastCellY = -1;

    // Set up variables for fixed point texture coordinates
    q16_16 texPosX = 0;
    q16_16 texPosY = 0;

    // Set up variables for integer texture indices
    uint32_t tx = 0;
    uint32_t ty = 0;

    // Set a pointer for textures later
    paletteColor_t* texture = NULL;
    // The ceiling texture is always this
    paletteColor_t* ceilTexture = getTexByType(ray, BG_CEILING)->px;

    // Save these to not resolve pointers later
    uint32_t mapW = ray->map.w;
    uint32_t mapH = ray->map.h;

    // rayDir for leftmost ray (x = 0) and rightmost ray (x = w)
    q24_8 rayDirX0 = SUB_FX(ray->dirX, ray->planeX);
    q24_8 rayDirY0 = SUB_FX(ray->dirY, ray->planeY);
    q24_8 rayDirX1 = ADD_FX(ray->dirX, ray->planeX);
    q24_8 rayDirY1 = ADD_FX(ray->dirY, ray->planeY);

    // Loop through each horizontal row
    for (int32_t y = firstRow; y < lastRow; y++)
    {
        // Are we casting on the floor or ceiling?
        bool isFloor = y > (TFT_HEIGHT / 2);

        // Current y position compared to the center of the screen (the horizon)
        int32_t p;
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
            camZ = ADD_FX(TO_FX_FRAC(TFT_HEIGHT, 2), ray->posZ);
        }
        else
        {
            camZ = SUB_FX(TO_FX_FRAC(TFT_HEIGHT, 2), ray->posZ);
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
        q16_16 floorX = ADD_FX(ray->posX, MUL_FX(rowDistance, rayDirX0)) << EX_CEIL_PRECISION_BITS;
        q16_16 floorY = ADD_FX(ray->posY, MUL_FX(rowDistance, rayDirY0)) << EX_CEIL_PRECISION_BITS;

        // calculate the real world step vector we have to add for each x (parallel to camera plane)
        // adding step by step avoids multiplications with a weight in the inner loop
        q16_16 floorStepX = (MUL_FX(rowDistance, SUB_FX(rayDirX1, rayDirX0)) << EX_CEIL_PRECISION_BITS) / TFT_WIDTH;
        q16_16 floorStepY = (MUL_FX(rowDistance, SUB_FX(rayDirY1, rayDirY0)) << EX_CEIL_PRECISION_BITS) / TFT_WIDTH;

        // This is the fixed point amount each texture coordinate increments per screen pixel
        q16_16 texStepX = TEX_WIDTH * floorStepX;
        q16_16 texStepY = TEX_HEIGHT * floorStepY;

        // Loop through each pixel
        for (int32_t x = 0; x < TFT_WIDTH; ++x)
        {
            // the cell coord is simply got from the integer parts of floorX and floorY
            cellX = floorX >> Q16_16_FRAC_BITS;
            cellY = floorY >> Q16_16_FRAC_BITS;

            // Only draw floor and ceiling for valid cells, otherwise leave the pixel as-is
            if (cellX < mapW && cellY < mapH)
            {
                // If the cell changed
                if ((cellX != lastCellX) || (cellY != lastCellY))
                {
                    // Record the change
                    lastCellX = cellX;
                    lastCellY = cellY;

                    if (isFloor)
                    {
                        // Get the next cell texture
                        rayMapCellType_t type = ray->map.tiles[cellX][cellY].type;
                        // Always draw floor under doors
                        if (CELL_IS_TYPE(type, BG | DOOR))
                        {
                            type = BG_FLOOR;
                        }
                        texture = getTexByType(ray, type)->px;
                    }
                    else
                    {
                        texture = ceilTexture;
                    }

                    // get the texture coordinate from the fractional part
                    texPosX = (TEX_WIDTH * (floorX - (floorX & Q16_16_WHOLE_MASK)));
                    texPosY = (TEX_HEIGHT * (floorY - (floorY & Q16_16_WHOLE_MASK)));
                }

                // Get the integer texture indices from the fixed point texture position
                tx = (((uint32_t)texPosX) >> Q16_16_FRAC_BITS) % TEX_WIDTH;
                ty = (((uint32_t)texPosY) >> Q16_16_FRAC_BITS) % TEX_HEIGHT;

                // Draw the pixel
                if (isXray)
                {
                    TURBO_SET_PIXEL(x, y, xrayPaletteSwap[texture[TEX_WIDTH * ty + tx]]);
                }
                else
                {
                    TURBO_SET_PIXEL(x, y, texture[TEX_WIDTH * ty + tx]);
                }
            }

            // Always increment, regardless of if pixels were drawn
            floorX += floorStepX;
            floorY += floorStepY;

            // Increment the texture coordinate as well
            texPosX += texStepX;
            texPosY += texStepY;
        }
    }
}

/**
 * @brief Draw all the wall pixels. Walls are drawn second. This iterates over the screen left-to-right and draws
 * vertical columns.
 * This is called from the main loop.
 *
 * @param ray The entire game state
 */
void castWalls(ray_t* ray)
{
    // We'll be drawing pixels, so set this up
    SETUP_FOR_TURBO();

    // Boolean if the colors should be drawn inverted
    bool isXray = (LO_XRAY == ray->loadout);

    // For each ray
    for (int32_t x = 0; x < TFT_WIDTH; x++)
    {
        // calculate ray position and direction
        q24_8 cameraX = ((x * TO_FX(2)) / TFT_WIDTH) - TO_FX(1); // x-coordinate in camera space
        q24_8 rayDirX = ADD_FX(ray->dirX, MUL_FX(ray->planeX, cameraX));
        q24_8 rayDirY = ADD_FX(ray->dirY, MUL_FX(ray->planeY, cameraX));

        // which box of the map we're in
        int32_t mapX = FROM_FX(ray->posX);
        int32_t mapY = FROM_FX(ray->posY);

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
        q24_8 deltaDistX = (rayDirX == 0) ? INT32_MAX : ABS(DIV_FX(TO_FX(1), rayDirX));
        q24_8 deltaDistY = (rayDirY == 0) ? INT32_MAX : ABS(DIV_FX(TO_FX(1), rayDirY));

        // what direction to step in x or y-direction (either +1 or -1)
        int32_t stepX = 0;
        int32_t stepY = 0;

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

        q24_8 wallX        = 0;                             // where exactly the wall was hit
        int32_t lineHeight = 0, drawStart = 0, drawEnd = 0; // the height of the wall strip
        bool xrayOverride = false;                          // Whether or not a wall should be drawn instead of a door
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
            rayMapCellType_t tileType = ray->map.tiles[mapX][mapY].type;
            if (CELL_IS_TYPE(tileType, BG | WALL) || CELL_IS_TYPE(tileType, BG | DOOR))
            {
                // Check if the door should be drawn recessed or not
                bool drawRecessedDoor = false;
                if (tileType == BG_DOOR_XRAY)
                {
                    // X-Ray door, only draw recessed if the X-Ray loadout is active or the door is open
                    if ((LO_XRAY == ray->loadout) || (TO_FX(1) == ray->map.tiles[mapX][mapY].doorOpen))
                    {
                        // Draw recessed door
                        drawRecessedDoor = true;
                    }
                    // If not fully open, draw X-Ray doors as walls without the X-Ray loadout
                    else
                    {
                        xrayOverride = true;
                    }
                }
                else if (CELL_IS_TYPE(tileType, BG | DOOR))
                {
                    // Not an X-Ray door, always draw recessed
                    drawRecessedDoor = true;
                }

                // If this cell is a door
                if (drawRecessedDoor)
                {
                    // Check if the ray actually intersects the recessed door
                    if (rayIntersectsDoor(side, mapX, mapY, ray->posX, ray->posY, rayDirX, rayDirY, deltaDistX,
                                          deltaDistY, ray->map.tiles[mapX][mapY].doorOpen))
                    {
                        // Add a half step to these values to recess the door
                        sideDistX = ADD_FX(sideDistX, deltaDistX / 2);
                        sideDistY = ADD_FX(sideDistY, deltaDistY / 2);
                    }
                    else
                    {
                        // Didn't collide with a door, so keep DDA'ing
                        continue;
                    }
                }

                // Calculate distance projected on camera direction. This is the shortest distance from the point
                // where the wall is hit to the camera plane. Euclidean to center camera point would give fisheye
                // effect! This can be computed as (mapX - ray->posX + (1 - stepX) / 2) / rayDirX for side == 0, or
                // same formula with Y for size == 1, but can be simplified to the code below thanks to how sideDist
                // and deltaDist are computed: because they were left scaled to |rayDir|. sideDist is the entire
                // length of the ray above after the multiple steps, but we subtract deltaDist once because one step
                // more into the wall was taken above.
                q24_8 perpWallDist;
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

                if (perpWallDist == 0)
                {
                    // Calculate height of line to draw on screen, make sure not to div by zero
                    lineHeight = TFT_HEIGHT;

                    // calculate lowest and highest pixel to fill in current stripe
                    drawStart = (TFT_HEIGHT - lineHeight) / 2;
                    drawEnd   = (TFT_HEIGHT + lineHeight) / 2;
                }
                else
                {
                    // Calculate height of line to draw on screen, make sure not to div by zero
                    lineHeight = TO_FX(TFT_HEIGHT) / perpWallDist;

                    // calculate lowest and highest pixel to fill in current stripe
                    drawStart = (TFT_HEIGHT - lineHeight) / 2 + (ray->posZ / perpWallDist);
                    drawEnd   = (TFT_HEIGHT + lineHeight) / 2 + (ray->posZ / perpWallDist);
                }

                // Sometimes textures wraparound b/c the math for wallX comes out to be like
                // 19.003 -> 0
                // 19.000 -> 0
                // 18.995 -> 63

                // calculate value of wallX
                if (false == side)
                {
                    wallX = ADD_FX(ray->posY, MUL_FX(perpWallDist, rayDirY));
                }
                else
                {
                    wallX = ADD_FX(ray->posX, MUL_FX(perpWallDist, rayDirX));
                }
                wallX = SUB_FX(wallX, FLOOR_FX(wallX));

                // For sliding doors
                if (drawRecessedDoor)
                {
                    // Adjust wallX to start drawing the texture at the door's edge rather than the map cell's edge
                    wallX -= ray->map.tiles[mapX][mapY].doorOpen;

                    // If this is negative, it would draw an out-of-bounds pixel.
                    // Negative numbers are a rounding error, so make it zero
                    if (wallX < 0)
                    {
                        wallX = 0;
                    }
                }

                // Wall or door was hit, this stops the DDA loop
                hit = true;
            }
        }

        // x coordinate on the texture
        int32_t texX = FROM_FX(wallX * TEX_WIDTH);

        // Mirror X texture coordinate for certain walls
        if ((false == side && rayDirX < 0) || (true == side && rayDirY > 0))
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
        if (xrayOverride)
        {
            tex = getTexByType(ray, BG_WALL_1)->px;
        }
        else
        {
            tex = getTexByType(ray, ray->map.tiles[mapX][mapY].type)->px;
        }

        // Draw a vertical strip
        for (int32_t y = drawStart; y < drawEnd; y++)
        {
            // Cast the texture coordinate to integer, and mod it to ensure no out of bounds reads
            int32_t texY = (texPos >> 24) % TEX_HEIGHT;

            // Increment the texture position for the next iteration
            texPos += step;

            // Get the color from the texture

            if (isXray)
            {
                TURBO_SET_PIXEL(x, y, xrayPaletteSwap[tex[TEX_HEIGHT * texY + texX]]);
            }
            else
            {
                TURBO_SET_PIXEL(x, y, tex[TEX_HEIGHT * texY + texX]);
            }
        }
    }
}

/**
 * @brief Check if a ray intersects with a door. Doors are recessed a half-cell back
 *
 * @param side true if the ray came through horizontal boundary, false if it came through a vertical boundary
 * @param mapX The player's current map cell X
 * @param mapY The player's current map cell Y
 * @param posX The player's current position X, fixed point decimal
 * @param posY The player's current position Y, fixed point decimal
 * @param rayDirX The X component of the ray being cast, fixed point decimal
 * @param rayDirY The Y component of the ray being cast, fixed point decimal
 * @param deltaDistX The X component of a DDA step, fixed point decimal
 * @param deltaDistY The Y component of a DDA step, fixed point decimal
 * @param doorOpen How open the door is, 0 to 1, fixed point decimal
 * @return true if the ray intersects the door, false if it doesn't
 */
static bool rayIntersectsDoor(bool side, int32_t mapX, int32_t mapY, q24_8 posX, q24_8 posY, q24_8 rayDirX,
                              q24_8 rayDirY, q24_8 deltaDistX, q24_8 deltaDistY, q24_8 doorOpen)
{
    // Avoid division by zero
    if (0 == rayDirX)
    {
        // Compare the decimal part of the camera X position to how open the door is
        if ((posX & (TO_FX(1) - 1)) >= doorOpen)
        {
            // Intersection!
            return true;
        }
    }
    else
    {
        // Find the B part of a line formula (y = m*x + b)
        q24_8 playerB = SUB_FX(posY, DIV_FX(MUL_FX(posX, rayDirY), rayDirX));

        // Do different checks whether the ray is coming through a horizontal or vertical boundary
        if (side)
        {
            // Check for intersection with 'horizontal' door, i.e. fixed Y
            // Avoid division by zero
            if (0 == rayDirY)
            {
                // Compare the decimal part of the camera Y position to how open the door is
                if ((posY & (TO_FX(1) - 1)) >= doorOpen)
                {
                    // Intersection
                    return true;
                }
            }
            else
            {
                // Find the door's Y so we can solve for X (recess it half a cell)
                q24_8 doorY;
                if (deltaDistY > 0)
                {
                    doorY = TO_FX(mapY) + TO_FX_FRAC(1, 2);
                }
                else
                {
                    doorY = TO_FX(mapY) - TO_FX_FRAC(1, 2);
                }

                // Solve for the X of the intersection
                q24_8 doorIntersectionX = DIV_FX(MUL_FX(SUB_FX(doorY, playerB), rayDirX), rayDirY);

                // If the intersection is in the same cell as the door
                if (FROM_FX(doorIntersectionX) == mapX)
                {
                    // Compare the decimal part of the intersection point with how open the door is
                    if ((doorIntersectionX & (TO_FX(1) - 1)) >= doorOpen)
                    {
                        // The ray intersects with the door
                        return true;
                    }
                }
            }
        }
        else
        {
            // 'Vertical' door, fixed X

            // Find the door's X so we can solve for Y (recess it half a cell)
            q24_8 doorX;
            if (deltaDistX > 0)
            {
                doorX = TO_FX(mapX) + TO_FX_FRAC(1, 2);
            }
            else
            {
                doorX = TO_FX(mapX) - TO_FX_FRAC(1, 2);
            }

            // Solve for the Y of the intersection
            q24_8 doorIntersectionY = ADD_FX(DIV_FX(MUL_FX(doorX, rayDirY), rayDirX), playerB);

            // If the intersection is in the same cell as the door
            if (FROM_FX(doorIntersectionY) == mapY)
            {
                // Compare the decimal part of the intersection point with how open the door is
                if ((doorIntersectionY & (TO_FX(1) - 1)) >= doorOpen)
                {
                    // The ray intersects with the door
                    return true;
                }
            }
        }
    }

    // No intersection
    return false;
}

/**
 * @brief Compare two objDist_t* based on distance
 *
 * @param obj1 A objDist_t* to compare
 * @param obj2 Another objDist_t* to compare
 * @return an integer less than, equal to, or greater than zero if the first
 *         argument is considered to be respectively less than, equal to, or
 *         greater than the second.
 */
static int objDistComparator(const void* obj1, const void* obj2)
{
    return (((const objDist_t*)obj2)->dist - ((const objDist_t*)obj1)->dist);
}

/**
 * @brief Draw all the sprites. Sprites are drawn third. This sorts all sprites from furthest away to closest, then
 * draws them on the screen.
 * This is called from the main loop.
 *
 * @param ray The entire game state
 * @return The closest sprite in the lock zone, i.e. center of the display. This may be NULL if there are no centered
 * sprites.
 */
rayObjCommon_t* castSprites(ray_t* ray)
{
    rayObjCommon_t* lockedObj = NULL;

    // Setup to draw
    SETUP_FOR_TURBO();

    // Boolean if the colors should be drawn inverted
    bool isXray = (LO_XRAY == ray->loadout);

    // Put an array on the stack to sort all sprites
    objDist_t allObjs[MAX_RAY_BULLETS + ray->scenery.length + ray->enemies.length + ray->items.length];
    int32_t allObjsIdx = 0;

    // For convenience
    q24_8 rayPosX = ray->posX;
    q24_8 rayPosY = ray->posY;

    // Assign each bullet a distance from the player
    for (int i = 0; i < MAX_RAY_BULLETS; i++)
    {
        // Make a convenience pointer
        rayBullet_t* obj = &ray->bullets[i];
        if (-1 != obj->c.id)
        {
            // Save the pointer and the distance to sort
            allObjs[allObjsIdx].obj  = &obj->c;
            q24_8 delX               = rayPosX - obj->c.posX;
            q24_8 delY               = rayPosY - obj->c.posY;
            allObjs[allObjsIdx].dist = (delX * delX) + (delY * delY);
            allObjsIdx++;
        }
    }

    // Assign each enemy a distance from the player
    node_t* currentNode = ray->enemies.first;
    while (currentNode != NULL)
    {
        // Get a pointer from the linked list
        rayEnemy_t* obj = ((rayEnemy_t*)currentNode->val);

        // Save the pointer and the distance to sort
        allObjs[allObjsIdx].obj  = &obj->c;
        q24_8 delX               = rayPosX - obj->c.posX;
        q24_8 delY               = rayPosY - obj->c.posY;
        allObjs[allObjsIdx].dist = (delX * delX) + (delY * delY);
        allObjsIdx++;

        // Iterate to the next node
        currentNode = currentNode->next;
    }

    // Assign each enemy a distance from the player
    currentNode = ray->scenery.first;
    while (currentNode != NULL)
    {
        // Get a pointer from the linked list
        rayObjCommon_t* obj = ((rayObjCommon_t*)currentNode->val);

        // Save the pointer and the distance to sort
        allObjs[allObjsIdx].obj  = obj;
        q24_8 delX               = rayPosX - obj->posX;
        q24_8 delY               = rayPosY - obj->posY;
        allObjs[allObjsIdx].dist = (delX * delX) + (delY * delY);
        allObjsIdx++;

        // Iterate to the next node
        currentNode = currentNode->next;
    }

    // Assign each item a distance from the player
    currentNode = ray->items.first;
    while (currentNode != NULL)
    {
        // Get a pointer from the linked list
        rayObjCommon_t* obj = ((rayObjCommon_t*)currentNode->val);

        // Save the pointer and the distance to sort
        allObjs[allObjsIdx].obj  = obj;
        q24_8 delX               = rayPosX - obj->posX;
        q24_8 delY               = rayPosY - obj->posY;
        allObjs[allObjsIdx].dist = (delX * delX) + (delY * delY);
        allObjsIdx++;

        // Iterate to the next node
        currentNode = currentNode->next;
    }

    // Sort the sprites by distance
    qsort(allObjs, allObjsIdx, sizeof(objDist_t), objDistComparator);

    // after sorting the sprites, do the projection and draw them
    for (int i = 0; i < allObjsIdx; i++)
    {
        // Make a convenience pointer
        rayObjCommon_t* obj = allObjs[i].obj;

        // Make sure this object slot is occupied
        if (-1 != obj->id)
        {
            // Get WSG dimensions for convenience
            uint32_t tWidth  = obj->sprite->w;
            uint32_t tHeight = obj->sprite->h;

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

            // Do all the X math first to see if its on screen, then do Y math??
            q24_8 transformX = DIV_FX(SUB_FX(MUL_FX(ray->dirY, spriteX), MUL_FX(ray->dirX, spriteY)), invDetDivisor);

            // The center of the sprite in screen space
            //  The division here takes the number from q24_8 to int32_t
            int32_t spriteScreenX = (TFT_WIDTH * (transformX + transformY)) / (2 * transformY);

            // The width of the screen area to draw the sprite into, in pixels
            int32_t spriteWidth = (tWidth * TO_FX(TFT_HEIGHT)) / (TEX_WIDTH * transformY);
            if (0 == spriteWidth)
            {
                // If this sprite has zero width, don't draw it
                continue;
            }
            // Width should always be positive
            if (spriteWidth < 0)
            {
                spriteWidth = -spriteWidth;
            }

            // This is the texture step per-screen-pixel
            q16_16 texXDelta = (tWidth << 16) / spriteWidth;
            // This is the inital texture X coordinate
            q16_16 texX = 0;

            // Find the pixel X coordinate where the sprite draw starts. It may be negative
            int32_t drawStartX = spriteScreenX - (spriteWidth / 2);
            // If the sprite would start to draw off-screen
            if (drawStartX < 0)
            {
                // Advance the initial texture X coordinate by the difference
                texX = texXDelta * -drawStartX;
                // Start drawing at the screen edge
                drawStartX = 0;
            }
            // Find the pixel X coordinate where the sprite draw ends. It may be off the screen
            int32_t drawEndX = spriteScreenX + (spriteWidth / 2);
            if (drawEndX > TFT_WIDTH)
            {
                // Always stop drawing at the screen edge
                drawEndX = TFT_WIDTH;
            }

            if (drawStartX >= TFT_WIDTH || drawEndX < 0)
            {
                // Not drawn in bounds, so continue to the next object
                continue;
            }

            // Mirrored sprites draw backwards
            if (obj->spriteMirrored)
            {
                texXDelta = -texXDelta;
                texX      = (tWidth << 16) - texX - 1;
            }

            // Adjust the sprite draw based on the vertical camera height.
            // Dividing two q24_8 variables gets a int32_t
            int32_t spritePosZ = ray->posZ / transformY;

            // calculate height of the sprite on screen
            // using 'transformY' instead of the real distance prevents fisheye
            int32_t spriteHeight = TO_FX(TFT_HEIGHT) / transformY;
            if (spriteHeight < 0)
            {
                spriteHeight = -spriteHeight;
            }

            // This is the texture step per-screen-pixel
            q16_16 texYDelta = (tHeight << 16) / spriteHeight;
            // This is the inital texture Y coordinate
            q16_16 initialTexY = 0;

            // Find the pixel Y coordinate where the sprite draw starts. It may be negative
            int32_t drawStartY = (-spriteHeight + TFT_HEIGHT) / 2 + spritePosZ;
            if (drawStartY < 0)
            {
                // Advance the initial texture Y coordinate by the difference
                initialTexY = texYDelta * -drawStartY;
                // Start drawing at the screen edge
                drawStartY = 0;
            }

            // Find the pixel Y coordinate where the sprite draw ends. It may be off the screen
            int32_t drawEndY = (spriteHeight + TFT_HEIGHT) / 2 + spritePosZ;
            if (drawEndY > TFT_HEIGHT)
            {
                // Always stop drawing at the screen edge
                drawEndY = TFT_HEIGHT;
            }

            // loop through every vertical stripe of the sprite on screen
            for (int32_t stripe = drawStartX; stripe < drawEndX; stripe++)
            {
                // Check wallDistBuffer to make sure the sprite is on the screen
                if (transformY < ray->wallDistBuffer[stripe])
                {
                    // Check if this should be locked onto
                    if (((TFT_WIDTH / 2) - LOCK_ZONE) <= stripe && stripe <= ((TFT_WIDTH / 2) + LOCK_ZONE))
                    {
                        // Closest sprites are drawn last, so override the lock
                        lockedObj = obj;
                    }

                    // Reset the texture Y coordinate
                    q16_16 texY = initialTexY;

                    // for every pixel of the current stripe
                    for (int32_t y = drawStartY; y < drawEndY; y++)
                    {
                        // get current color from the texture, draw if not transparent
                        paletteColor_t color = obj->sprite->px[tWidth * (texY >> 16) + (texX >> 16)];
                        if (cTransparent != color)
                        {
                            if (isXray)
                            {
                                TURBO_SET_PIXEL(stripe, y, xrayPaletteSwap[color]);
                            }
                            else
                            {
                                TURBO_SET_PIXEL(stripe, y, color);
                            }
                        }
                        texY += texYDelta;
                    }
                }
                texX += texXDelta;
            }
        }
    }
    return lockedObj;
}

/**
 * @brief Draw the HUD on the display based on the player loadout. This is drawn last.
 * This is called from the main loop.
 *
 * @param ray The entire game state
 */
void drawHud(ray_t* ray)
{
    if (LO_NONE != ray->loadout)
    {
        wsg_t* gun      = &ray->guns[ray->loadout];
        int32_t yOffset = TFT_HEIGHT - gun->h;
        // If a loadout change is in progress
        if (ray->loadoutChangeTimer)
        {
            // Loadout is changing out
            if (ray->loadout != ray->nextLoadout)
            {
                // Timer goes from LOADOUT_TIMER_US to 0, as it gets smaller the gun moves down
                yOffset += (gun->h - ((ray->loadoutChangeTimer * gun->h) / LOADOUT_TIMER_US));
            }
            // Loadout is changing in
            else
            {
                // Timer goes from LOADOUT_TIMER_US to 0, as it gets smaller the gun moves up
                yOffset += ((ray->loadoutChangeTimer * gun->h) / LOADOUT_TIMER_US);
            }
        }
        drawWsgSimple(gun, TFT_WIDTH - gun->w, yOffset);
    }

    // If the player has missiles
    if (ray->inventory.missileLoadOut)
    {
        // Draw a count of missiles
        char missileStr[16] = {0};
        snprintf(missileStr, sizeof(missileStr) - 1, "%03" PRId32 "/%03" PRId32, ray->inventory.numMissiles,
                 ray->inventory.maxNumMissiles);
        drawText(&ray->ibm, c555, missileStr, 64, TFT_HEIGHT - ray->ibm.height);
    }

#define BAR_END_MARGIN  40
#define BAR_SIDE_MARGIN 8
#define BAR_WIDTH       8
    // Find the width of the entire health bar
    int32_t maxHealthWidth = (ray->inventory.maxHealth * (TFT_WIDTH - (BAR_END_MARGIN * 2))) / MAX_HEALTH_EVER;
    // Find the width of the filled part of the health bar
    int32_t currHealthWidth = (ray->inventory.health * (TFT_WIDTH - (BAR_END_MARGIN * 2))) / MAX_HEALTH_EVER;
    // Draw a health bar
    fillDisplayArea(BAR_END_MARGIN,                   //
                    BAR_SIDE_MARGIN,                  //
                    BAR_END_MARGIN + currHealthWidth, //
                    BAR_SIDE_MARGIN + BAR_WIDTH,      //
                    c030);
    fillDisplayArea(BAR_END_MARGIN + currHealthWidth, //
                    BAR_SIDE_MARGIN,                  //
                    BAR_END_MARGIN + maxHealthWidth,  //
                    BAR_SIDE_MARGIN + BAR_WIDTH,      //
                    c400);

    // Draw charge beam indicator
    int32_t chargeIndicatorStart
        = TFT_HEIGHT - BAR_END_MARGIN - ((ray->chargeTimer * (TFT_HEIGHT - (2 * BAR_END_MARGIN))) / CHARGE_TIME_US);
    fillDisplayArea(BAR_SIDE_MARGIN,             //
                    chargeIndicatorStart,        //
                    BAR_SIDE_MARGIN + BAR_WIDTH, //
                    TFT_HEIGHT - BAR_END_MARGIN, //
                    c550);
    fillDisplayArea(TFT_WIDTH - BAR_SIDE_MARGIN - BAR_WIDTH, //
                    chargeIndicatorStart,                    //
                    TFT_WIDTH - BAR_SIDE_MARGIN,             //
                    TFT_HEIGHT - BAR_END_MARGIN,             //
                    c550);

    // Draw side bars according to suit colors
    paletteColor_t sideBarColor = c432;
    if (ray->inventory.waterSuit)
    {
        sideBarColor = c223;
    }
    else if (ray->inventory.lavaSuit)
    {
        sideBarColor = c510;
    }
    fillDisplayArea(BAR_SIDE_MARGIN,             //
                    BAR_END_MARGIN,              //
                    BAR_SIDE_MARGIN + BAR_WIDTH, //
                    chargeIndicatorStart,        //
                    sideBarColor);
    fillDisplayArea(TFT_WIDTH - BAR_SIDE_MARGIN - BAR_WIDTH, //
                    BAR_END_MARGIN,                          //
                    TFT_WIDTH - BAR_SIDE_MARGIN,             //
                    chargeIndicatorStart,                    //
                    sideBarColor);
}

/**
 * @brief Run all environment timers, including door openings and head-bob
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void runEnvTimers(ray_t* ray, uint32_t elapsedUs)
{
    // Run a timer for head bob
    ray->bobTimer += elapsedUs;
    while (ray->bobTimer > 2500)
    {
        ray->bobTimer -= 2500;

        // Only bob when walking or finishing a bob cycle
        if ((ray->btnState & (PB_UP | PB_DOWN)) || (0 != ray->bobCount && 180 != ray->bobCount))
        {
            // Step through the bob cycle, which is a sin function
            ray->bobCount++;
            if (360 == ray->bobCount)
            {
                ray->bobCount = 0;
            }
            // Bob the camera. Note that fixed point numbers are << 8, and trig functions are << 10
            ray->posZ = getSin1024(ray->bobCount) * 4;
        }
        else
        {
            // Reset the count to always restart on an upward bob
            ray->bobCount = 0;
        }
    }

    // Run a timer to open and close doors
    ray->doorTimer += elapsedUs;
    while (ray->doorTimer >= 5000)
    {
        ray->doorTimer -= 5000;

        for (int32_t y = 0; y < ray->map.h; y++)
        {
            for (int32_t x = 0; x < ray->map.w; x++)
            {
                if (ray->map.tiles[x][y].doorOpen > 0 && ray->map.tiles[x][y].doorOpen < TO_FX(1))
                {
                    ray->map.tiles[x][y].doorOpen++;
                }
            }
        }
    }
}
