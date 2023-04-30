//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include "mode_ray.h"

//==============================================================================
// Defines
//==============================================================================

#define TEX_WIDTH  64
#define TEX_HEIGHT 64
#define MAP_WIDTH  24
#define MAP_HEIGHT 24

#define FRAC_BITS              8
#define EX_CEIL_PRECISION_BITS 8
#define EX_CEIL_PRECISION      (1 << EX_CEIL_PRECISION_BITS)

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    wsg_t texFloor;
    wsg_t texWall;
    wsg_t texCeiling;

    int32_t posX;
    int32_t posY;
    int32_t dirX;
    int32_t dirY;
    int32_t planeX;
    int32_t planeY;
    int32_t dirAngle;

    uint16_t btnState;
    uint16_t* floorTexIdx;
} ray_t;

//==============================================================================
// Function Prototypes
//==============================================================================

void rayEnterMode(void);
void rayExitMode(void);
void rayMainLoop(int64_t elapsedUs);
void rayBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

void castFloorCeiling(int16_t firstRow, int16_t lastRow);
void castWalls(void);

//==============================================================================
// Const Variables
//==============================================================================

const char rayName[] = "Ray";
const uint8_t worldMap[MAP_WIDTH][MAP_HEIGHT]
    = {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 0, 0, 0, 0, 3, 0, 3, 0, 3, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 2, 2, 0, 2, 2, 0, 0, 0, 0, 3, 0, 3, 0, 3, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 4, 0, 4, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 4, 0, 0, 0, 0, 5, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 4, 0, 4, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 4, 0, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
       {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};

//==============================================================================
// Variables
//==============================================================================

swadgeMode_t rayMode = {
    .modeName                 = rayName,
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .fnEnterMode              = rayEnterMode,
    .fnExitMode               = rayExitMode,
    .fnMainLoop               = rayMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = rayBackgroundDrawCallback,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

ray_t* ray;

//==============================================================================
// Fixed Point Math Functions
//==============================================================================

static inline int32_t TO_FX(double in)
{
    return (int32_t)(in * (1 << FRAC_BITS));
}

static inline int32_t FROM_FX(int32_t in)
{
    return in / (1 << FRAC_BITS);
}

static inline int32_t ADD_FX(int32_t a, int32_t b)
{
    return a + b;
}

static inline int32_t SUB_FX(int32_t a, int32_t b)
{
    return a - b;
}

static inline int32_t MUL_FX(int32_t a, int32_t b)
{
    return ((a * b) + (1 << (FRAC_BITS - 1))) / (1 << FRAC_BITS);
}

static inline int32_t DIV_FX(int32_t a, int32_t b)
{
    return ((a * (1 << FRAC_BITS)) / b);
}

static inline int32_t FLOOR_FX(int32_t a)
{
    return a & (~((1 << FRAC_BITS) - 1));
}

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void rayEnterMode(void)
{
    // Allocate memory
    ray              = calloc(1, sizeof(ray_t));
    ray->floorTexIdx = heap_caps_calloc(TFT_WIDTH * TFT_HEIGHT / 2, sizeof(uint16_t), MALLOC_CAP_SPIRAM);

    // Set initial position and direction
    ray->posX = (22 << 8), ray->posY = (12 << 8);
    ray->dirX = 0, ray->dirY = 0;
    ray->planeX = 0, ray->planeY = 0;
    ray->dirAngle = 0;

    // Load textures
    loadWsg("floor.wsg", &ray->texFloor, true);
    loadWsg("wall.wsg", &ray->texWall, true);
    loadWsg("ceiling.wsg", &ray->texCeiling, true);
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void rayExitMode(void)
{
    freeWsg(&ray->texFloor);
    freeWsg(&ray->texWall);
    freeWsg(&ray->texCeiling);
    free(ray->floorTexIdx);
    free(ray);
}

/**
 * @brief This function is called from the main loop. It's pretty quick, but the timing may be inconsistent.
 *
 * @param elapsedUs The time elapsed since the last time this function was called. Use this value to determine when
 * it's time to do things
 */
void rayMainLoop(int64_t elapsedUs)
{
    // Check all queued button events
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        ray->btnState = evt.state;
    }

    // Find move distances
    // TODO scale with elapsed time
    int32_t deltaX = (ray->dirX * 32) / 256; // MUL_FX(ray->dirX, 1);
    int32_t deltaY = (ray->dirY * 32) / 256; // MUL_FX(ray->dirY, 1);

    // Move forwards if no wall in front of you
    if (ray->btnState & PB_UP)
    {
        if (worldMap[FROM_FX(ray->posX + deltaX)][FROM_FX(ray->posY)] == false)
        {
            ray->posX += deltaX;
        }
        if (worldMap[FROM_FX(ray->posX)][FROM_FX(ray->posY + deltaY)] == false)
        {
            ray->posY += deltaY;
        }
    }

    // move backwards if no wall behind you
    if (ray->btnState & PB_DOWN)
    {
        if (worldMap[FROM_FX(ray->posX - deltaX)][FROM_FX(ray->posY)] == false)
        {
            ray->posX -= deltaX;
        }
        if (worldMap[FROM_FX(ray->posX)][FROM_FX(ray->posY - deltaY)] == false)
        {
            ray->posY -= deltaY;
        }
    }

    // Rotate right
    if (ray->btnState & PB_RIGHT)
    {
        ray->dirAngle = ADD_FX(ray->dirAngle, TO_FX(3));
        if (ray->dirAngle >= TO_FX(360))
        {
            ray->dirAngle -= TO_FX(360);
        }
    }

    // Rotate left
    if (ray->btnState & PB_LEFT)
    {
        ray->dirAngle = SUB_FX(ray->dirAngle, TO_FX(3));
        if (ray->dirAngle < TO_FX(0))
        {
            ray->dirAngle += TO_FX(360);
        }
    }

    // Draw the walls. The background is already drawn in rayBackgroundDrawCallback()
    castWalls();
}

/**
 * @brief This function is called when the display driver wishes to update a section of the display.
 *
 * @param disp The display to draw to
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
void rayBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Draw a portion of the background
    castFloorCeiling(y, y + h);
}

/**
 * @brief TODO
 *
 * @param firstRow
 * @param lastRow
 */
void castFloorCeiling(int16_t firstRow, int16_t lastRow)
{
    // We'll be drawing pixels, so set this up
    SETUP_FOR_TURBO();

    // Loop through each horizontal row
    for (int16_t yLoop = firstRow; yLoop < lastRow; yLoop++)
    {
        // If this is drawing the floor, don't do the math again, just use the
        // texture coordinates calculated from the ceiling
        if (yLoop > TFT_HEIGHT / 2)
        {
            // Get a pointer to this row's indices
            uint16_t* floorTexIdxY = &ray->floorTexIdx[(TFT_HEIGHT - yLoop - 1) * TFT_WIDTH];

            // Loop through each pixel
            for (int16_t x = 0; x < TFT_WIDTH; x++)
            {
                // Draw the pixel from the texture to the background
                TURBO_SET_PIXEL(x, yLoop, ray->texFloor.px[floorTexIdxY[x]]);
            }
        }
        // If this is drawing the ceiling, calculate texture coordinates
        else if (yLoop < TFT_HEIGHT / 2)
        {
            // The math in the example doesn't calculate correctly for the ceiling, so calculate the 'floor' instead!
            int16_t y = TFT_HEIGHT - yLoop - 1;

            // rayDir for leftmost ray (x = 0) and rightmost ray (x = w)
            int32_t rayDirX0 = SUB_FX(ray->dirX, ray->planeX);
            int32_t rayDirY0 = SUB_FX(ray->dirY, ray->planeY);

            // Vertical position of the camera.
            int16_t posZ = TFT_HEIGHT / 2;

            // Current y position compared to the center of the screen (the horizon)
            int16_t p = y - posZ;

            // Don't divide by zero (infinite distance on the horizon)
            if (0 == p)
            {
                continue;
            }

            // Calculate the amount to step each texture coordinate
            int32_t txfStep = ((posZ * 2 * ray->planeX * TEX_WIDTH) * EX_CEIL_PRECISION) / (TFT_WIDTH * p);
            int32_t tyfStep = ((posZ * 2 * ray->planeY * TEX_WIDTH) * EX_CEIL_PRECISION) / (TFT_WIDTH * p);

            // real world coordinates of the leftmost column.
            int32_t floorX = (ray->posX * EX_CEIL_PRECISION) + ((posZ * rayDirX0 * EX_CEIL_PRECISION) / p);
            int32_t floorY = (ray->posY * EX_CEIL_PRECISION) + ((posZ * rayDirY0 * EX_CEIL_PRECISION) / p);

            // Calculate the coordinate to start the texture at.  This will be updated as we step to the right.
            int32_t txf = (TEX_WIDTH * (floorX - (floorX & ~((1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS)) - 1))));
            int32_t tyf = (TEX_WIDTH * (floorY - (floorY & ~((1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS)) - 1))));

            // Loop through each pixel
            for (int16_t x = 0; x < TFT_WIDTH; ++x)
            {
                // Get the integer texture coordinate from the float
                uint16_t tx = (txf / (1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS))) % TEX_WIDTH;
                uint16_t ty = (tyf / (1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS))) % TEX_HEIGHT;

                // Step and modulo the texture X coordinate
                txf += txfStep;
                while (txf >= TO_FX(TEX_WIDTH * EX_CEIL_PRECISION))
                {
                    txf -= TO_FX(TEX_WIDTH * EX_CEIL_PRECISION);
                }
                while (txf < 0)
                {
                    txf += TO_FX(TEX_WIDTH * EX_CEIL_PRECISION);
                }

                // Step and modulo the texture Y coordinate
                tyf += tyfStep;
                while (tyf >= TO_FX(TEX_HEIGHT * EX_CEIL_PRECISION))
                {
                    tyf -= TO_FX(TEX_HEIGHT * EX_CEIL_PRECISION);
                }
                while (tyf < 0)
                {
                    tyf += TO_FX(TEX_HEIGHT * EX_CEIL_PRECISION);
                }

                // Draw the pixel
                TURBO_SET_PIXEL(x, yLoop, ray->texCeiling.px[TEX_WIDTH * ty + tx]);

                // Save the texture coordinate for the floor
                ray->floorTexIdx[(yLoop * TFT_WIDTH) + x] = TEX_WIDTH * ty + tx;
            }
        }
    }
}

/**
 * @brief TODO
 *
 */
void castWalls(void)
{
    // We'll be drawing pixels, so set this up
    SETUP_FOR_TURBO();

    // Compute cartesian direction and camera plane from direction
    // direction vector, cartesian coordinates
    ray->dirX = -getCos1024(FROM_FX(ray->dirAngle)) / 4; // trig functions are already << 10, so >> 2 to get to << 8
    ray->dirY = getSin1024(FROM_FX(ray->dirAngle)) / 4;

    // the 2d rayCaster version of camera plane, orthogonal to the direction vector
    ray->planeX = MUL_FX(TO_FX(0.66f), ray->dirY);
    ray->planeY = MUL_FX(TO_FX(-0.66f), ray->dirX);

    // For each ray
    for (int x = 0; x < TFT_WIDTH; x++)
    {
        // calculate ray position and direction
        int32_t cameraX = ((x * (2 << FRAC_BITS)) / TFT_WIDTH) - (1 << FRAC_BITS); // x-coordinate in camera space
        int32_t rayDirX = ADD_FX(ray->dirX, MUL_FX(ray->planeX, cameraX));
        int32_t rayDirY = ADD_FX(ray->dirY, MUL_FX(ray->planeY, cameraX));

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
        int32_t deltaDistX = (rayDirX == 0) ? INT32_MAX : ABS(DIV_FX((1 << FRAC_BITS), rayDirX));
        int32_t deltaDistY = (rayDirY == 0) ? INT32_MAX : ABS(DIV_FX((1 << FRAC_BITS), rayDirY));

        // what direction to step in x or y-direction (either +1 or -1)
        int8_t stepX = 0;
        int8_t stepY = 0;

        // length of ray from current position to next x or y-side
        int32_t sideDistX = 0;
        int32_t sideDistY = 0;

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

            // Check if ray has hit a wall
            if (worldMap[mapX][mapY] > 0)
            {
                hit = true;
            }
        }

        // Calculate distance projected on camera direction. This is the shortest distance from the point where the
        // wall is hit to the camera plane. Euclidean to center camera point would give fisheye effect! This can be
        // computed as (mapX - ray->posX + (1 - stepX) / 2) / rayDirX for side == 0, or same formula with Y for size ==
        // 1, but can be simplified to the code below thanks to how sideDist and deltaDist are computed: because
        // they were left scaled to |rayDir|. sideDist is the entire length of the ray above after the multiple
        // steps, but we subtract deltaDist once because one step more into the wall was taken above.
        int32_t perpWallDist;
        if (false == side)
        {
            perpWallDist = SUB_FX(sideDistX, deltaDistX);
        }
        else
        {
            perpWallDist = SUB_FX(sideDistY, deltaDistY);
        }

        // Calculate height of line to draw on screen, make sure not to div by zero
        int16_t lineHeight = (0 == perpWallDist) ? (TFT_HEIGHT) : (TO_FX(TFT_HEIGHT) / perpWallDist);

        // calculate lowest and highest pixel to fill in current stripe
        int16_t drawStart = (TFT_HEIGHT - lineHeight) / 2;
        if (drawStart < 0)
        {
            drawStart = 0;
        }
        int16_t drawEnd = (TFT_HEIGHT + lineHeight) / 2;
        if (drawEnd >= TFT_HEIGHT)
        {
            drawEnd = TFT_HEIGHT - 1;
        }

        // calculate value of wallX
        int32_t wallX; // where exactly the wall was hit
        if (side == 0)
        {
            wallX = ADD_FX(ray->posY, MUL_FX(perpWallDist, rayDirY));
        }
        else
        {
            wallX = ADD_FX(ray->posX, MUL_FX(perpWallDist, rayDirX));
        }
        wallX = SUB_FX(wallX, FLOOR_FX(wallX));

        // x coordinate on the texture
        int8_t texX = FROM_FX(wallX * TEX_WIDTH);

        // Mirror X texture coordinate for certain walls
        if ((side == 0 && rayDirX > 0) || (side == 1 && rayDirY < 0))
        {
            texX = TEX_WIDTH - texX - 1;
        }

        // How much to increase the texture coordinate per screen pixel
        int32_t step = (TEX_HEIGHT << 24) / lineHeight;
        // Starting texture coordinate
        int32_t texPos = 0;
        if (lineHeight > TFT_HEIGHT)
        {
            texPos = (((lineHeight - TFT_HEIGHT) * step) / 2);
        }

        // Draw a vertical strip
        for (int32_t y = drawStart; y < drawEnd; y++)
        {
            // Cast the texture coordinate to integer, and mask with (TEX_HEIGHT - 1) in case of overflow
            int8_t texY = (texPos >> 24) % TEX_HEIGHT;

            // Increment the texture position for the next iteration
            texPos += step;

            // Get the color from the texture
            TURBO_SET_PIXEL(x, y, ray->texWall.px[TEX_HEIGHT * texY + texX]);
        }
    }
}
