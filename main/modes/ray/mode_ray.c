//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include "mode_ray.h"
#include "spiffs_rmh.h"

//==============================================================================
// Defines
//==============================================================================

#define TEX_WIDTH  64
#define TEX_HEIGHT 64

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
    wsg_t texDoor;

    rayMap_t map;

    int32_t posX;
    int32_t posY;
    int32_t dirX;
    int32_t dirY;
    int32_t planeX;
    int32_t planeY;
    int32_t dirAngle;
    int32_t posZ;

    int32_t bobTimer;
    int16_t bobCount;

    uint16_t btnState;

    int16_t doorOpen;
    int32_t doorTimer;
    bool doorOpening;
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
static bool isPassableCell(rayMapCellType_t cell);

//==============================================================================
// Const Variables
//==============================================================================

const char rayName[] = "Ray";

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

static inline int32_t TO_FX(int32_t in)
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

// #define FMT_FX    "%s%d.%03d"
// #define STR_FX(x) ((x) < 0 ? "-" : ""), ABS(FROM_FX(x)), DEC_PART(x)

// static inline int32_t DEC_PART(int32_t in)
// {
//     return (1000 * (int64_t)(ABS(in) & ((1 << FRAC_BITS) - 1))) / (1 << FRAC_BITS);
// }

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void rayEnterMode(void)
{
    // Allocate memory
    ray = calloc(1, sizeof(ray_t));

    // Load the map
    loadRmh("demo.rmh", &ray->map, &ray->posX, &ray->posY, false);

    // Set initial position and direction
    ray->posX     = TO_FX(ray->posX);
    ray->posY     = TO_FX(ray->posY);
    ray->dirAngle = 0;
    ray->dirX = 0, ray->dirY = 0;
    ray->planeX = 0, ray->planeY = 0;
    ray->posZ = TO_FX(0);

    // Load textures
    loadWsg("floor.wsg", &ray->texFloor, true);
    loadWsg("wall.wsg", &ray->texWall, true);
    loadWsg("ceiling.wsg", &ray->texCeiling, true);
    loadWsg("door.wsg", &ray->texDoor, true);

    // Create an array for all LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    // Set the LED output
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * @brief This function is called when the mode is exited. It should free any allocated memory.
 */
void rayExitMode(void)
{
    freeWsg(&ray->texFloor);
    freeWsg(&ray->texWall);
    freeWsg(&ray->texCeiling);
    freeWsg(&ray->texDoor);
    freeRmh(&ray->map);
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
    // Draw the walls. The background is already drawn in rayBackgroundDrawCallback()
    castWalls();

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

    // Check all queued button events
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        ray->btnState = evt.state;
    }

    // Run a timer to open and close doors
    ray->doorTimer += elapsedUs;
    while (ray->doorTimer >= 5000)
    {
        ray->doorTimer -= 5000;
        if (0 < ray->doorOpen && ray->doorOpen < TO_FX(1))
        {
            if (ray->doorOpening)
            {
                ray->doorOpen++;
            }
            else
            {
                ray->doorOpen--;
            }
        }
    }

    // Boundary checks are longer than the move dist to not get right up on the wall
    int32_t boundaryCheckX = ray->dirX / 3;
    int32_t boundaryCheckY = ray->dirY / 3;
    // Find move distances
    // TODO scale with elapsed time
    int32_t deltaX = (ray->dirX / 6);
    int32_t deltaY = (ray->dirY / 6);

    // Move forwards if no wall in front of you
    if (ray->btnState & PB_UP)
    {
        if (isPassableCell(ray->map.tiles[FROM_FX(ray->posX + boundaryCheckX)][FROM_FX(ray->posY)]))
        {
            ray->posX += deltaX;
        }

        if (isPassableCell(ray->map.tiles[FROM_FX(ray->posX)][FROM_FX(ray->posY + boundaryCheckY)]))
        {
            ray->posY += deltaY;
        }
    }

    // move backwards if no wall behind you
    if (ray->btnState & PB_DOWN)
    {
        if (isPassableCell(ray->map.tiles[FROM_FX(ray->posX - boundaryCheckX)][FROM_FX(ray->posY)]))
        {
            ray->posX -= deltaX;
        }

        if (isPassableCell(ray->map.tiles[FROM_FX(ray->posX)][FROM_FX(ray->posY - boundaryCheckY)]))
        {
            ray->posY -= deltaY;
        }
    }

    // Rotate right
    if (ray->btnState & PB_RIGHT)
    {
        ray->dirAngle = ADD_FX(ray->dirAngle, TO_FX(5));
        if (ray->dirAngle >= TO_FX(360))
        {
            ray->dirAngle -= TO_FX(360);
        }
    }

    // Rotate left
    if (ray->btnState & PB_LEFT)
    {
        ray->dirAngle = SUB_FX(ray->dirAngle, TO_FX(5));
        if (ray->dirAngle < TO_FX(0))
        {
            ray->dirAngle += TO_FX(360);
        }
    }

    // Open or close doors
    if (ray->btnState & PB_A)
    {
        // Don't open or close door when standing on that tile
        if (BG_DOOR != ray->map.tiles[FROM_FX(ray->posX)][FROM_FX(ray->posY)])
        {
            if (0 == ray->doorOpen)
            {
                ray->doorOpening = true;
                ray->doorOpen    = 1;
            }
            else if (TO_FX(1) == ray->doorOpen)
            {
                ray->doorOpening = false;
                ray->doorOpen    = TO_FX(1) - 1;
            }
        }
    }
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
    for (int16_t y = firstRow; y < lastRow; y++)
    {
        bool isFloor = y > TFT_HEIGHT / 2;

        // rayDir for leftmost ray (x = 0) and rightmost ray (x = w)
        int32_t rayDirX0 = SUB_FX(ray->dirX, ray->planeX);
        int32_t rayDirY0 = SUB_FX(ray->dirY, ray->planeY);
        int32_t rayDirX1 = ADD_FX(ray->dirX, ray->planeX);
        int32_t rayDirY1 = ADD_FX(ray->dirY, ray->planeY);

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
        // matching also how the walls are being raycasted. For different values
        // than 0.5, a separate loop must be done for ceiling and floor since
        // they're no longer symmetrical.
        int32_t camZ;
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
        int32_t rowDistance = camZ / p;

        // real world coordinates of the leftmost column. This will be updated as we step to the right.
        int32_t floorX = ADD_FX(ray->posX, MUL_FX(rowDistance, rayDirX0)) * EX_CEIL_PRECISION;
        int32_t floorY = ADD_FX(ray->posY, MUL_FX(rowDistance, rayDirY0)) * EX_CEIL_PRECISION;

        // calculate the real world step vector we have to add for each x (parallel to camera plane)
        // adding step by step avoids multiplications with a weight in the inner loop
        int32_t floorStepX = (MUL_FX(rowDistance, SUB_FX(rayDirX1, rayDirX0)) * EX_CEIL_PRECISION) / TFT_WIDTH;
        int32_t floorStepY = (MUL_FX(rowDistance, SUB_FX(rayDirY1, rayDirY0)) * EX_CEIL_PRECISION) / TFT_WIDTH;

        // Loop through each pixel
        for (int16_t x = 0; x < TFT_WIDTH; ++x)
        {
            // the cell coord is simply got from the integer parts of floorX and floorY
            // int cellX = FROM_FX(floorX);
            // int cellY = FROM_FX(floorY);

            // get the texture coordinate from the fractional part
            int32_t fracPartX = floorX - (floorX & ~((1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS)) - 1));
            int32_t fracPartY = floorY - (floorY & ~((1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS)) - 1));
            uint16_t tx       = ((TEX_WIDTH * fracPartX) / (1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS))) % TEX_WIDTH;
            uint16_t ty       = ((TEX_HEIGHT * fracPartY) / (1 << (FRAC_BITS + EX_CEIL_PRECISION_BITS))) % TEX_HEIGHT;

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
 */
void castWalls(void)
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

        int32_t wallX;                          // where exactly the wall was hit
        int16_t lineHeight, drawStart, drawEnd; // the height of the wall strip
        int32_t perpWallDist;
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
        int32_t step = (TEX_HEIGHT << 24) / lineHeight;

        // Starting texture coordinate. If it would start offscreen, start it at the right spot onscreen instead
        int32_t texPos = 0;
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
        for (int32_t y = drawStart; y < drawEnd; y++)
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
 * @brief Check if a cell is currently passable
 *
 * @param cell The cell type to check
 * @return true if the cell can be passed through, false if it cannot
 */
static bool isPassableCell(rayMapCellType_t cell)
{
    switch (cell)
    {
        case BG_WALL:
        {
            // Never pass through walls
            return false;
        }
        case BG_DOOR:
        {
            // Only pass through open doors
            return (TO_FX(1) == ray->doorOpen);
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
            // Always pass through everything else
            return true;
        }
    }
}
