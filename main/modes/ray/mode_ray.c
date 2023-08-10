//==============================================================================
// Includes
//==============================================================================

#include <esp_heap_caps.h>
#include "mode_ray.h"
#include "ray_map_loader.h"
#include "ray_renderer.h"
#include "ray_object.h"

//==============================================================================
// Function Prototypes
//==============================================================================

void rayEnterMode(void);
void rayExitMode(void);
void rayMainLoop(int64_t elapsedUs);
void rayBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

static bool isPassableCell(rayMapCell_t* cell);
void setPlayerAngle(q24_8 angle);

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
// Functions
//==============================================================================

/**
 * @brief This function is called when this mode is started. It should initialize variables and start the mode.
 */
void rayEnterMode(void)
{
    // Allocate memory
    ray = calloc(1, sizeof(ray_t));

    // Set invalid IDs for all objects
    for (uint16_t objIdx = 0; objIdx < MAX_RAY_OBJS; objIdx++)
    {
        ray->objs[objIdx].id = -1;
    }

    // Load the map and object data
    loadRayMap("demo.rmh", &ray->map, ray->objs, &ray->posX, &ray->posY, false);

    // Set initial position and direction, centered on the tile
    ray->posX = TO_FX(ray->posX) + (1 << (FRAC_BITS - 1));
    ray->posY = TO_FX(ray->posY) + (1 << (FRAC_BITS - 1));
    setPlayerAngle(TO_FX(0));
    ray->posZ = TO_FX(0);

    // Load textures
    loadWsg("floor.wsg", &ray->texFloor, true);
    loadWsg("wall.wsg", &ray->texWall, true);
    loadWsg("ceiling.wsg", &ray->texCeiling, true);
    loadWsg("door.wsg", &ray->texDoor, true);

    loadWsg("golem.wsg", &ray->texGolem, true);
    loadWsg("dragon.wsg", &ray->texDragon, true);
    loadWsg("knight.wsg", &ray->texKnight, true);
    loadWsg("skeleton.wsg", &ray->texSkeleton, true);

    loadWsg("bullet.wsg", &ray->texBullet, true);

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

    freeWsg(&ray->texGolem);
    freeWsg(&ray->texDragon);
    freeWsg(&ray->texKnight);
    freeWsg(&ray->texSkeleton);

    freeWsg(&ray->texBullet);

    freeRayMap(&ray->map);
    free(ray);
}

/**
 * @brief Return texture for the cell type
 *
 * @param type
 * @return wsg_t*
 */
wsg_t* getTexture(rayMapCellType_t type)
{
    switch (type)
    {
        case EMPTY:
        case OBJ_DELETE:
        case OBJ_START_POINT:
        case OBJ_OBELISK:
        case OBJ_GUN:
        {
            return NULL;
        }
        case OBJ_BULLET:
        {
            return &ray->texBullet;
        }
        case BG_FLOOR:
        {
            return &ray->texFloor;
        }
        case BG_WALL:
        {
            return &ray->texWall;
        }
        case BG_CEILING:
        {
            return &ray->texCeiling;
        }
        case BG_DOOR:
        {
            return &ray->texDoor;
        }
        case OBJ_ENEMY_DRAGON:
        {
            return &ray->texDragon;
        }
        case OBJ_ENEMY_SKELETON:
        {
            return &ray->texSkeleton;
        }
        case OBJ_ENEMY_KNIGHT:
        {
            return &ray->texKnight;
        }
        case OBJ_ENEMY_GOLEM:
        {
            return &ray->texGolem;
        }
    }
    return NULL;
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
    uint16_t prevBtnState = ray->btnState;
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        ray->btnState = evt.state;
    }

    // Move objects, check logic, etc.
    moveRayObjects(ray, elapsedUs);
    // Draw the walls. The background is already drawn in rayBackgroundDrawCallback()
    castWalls(ray);
    // Draw sprites
    rayObj_t* centeredSprite = castSprites(ray);

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

        for (int16_t y = 0; y < ray->map.h; y++)
        {
            for (int16_t x = 0; x < ray->map.w; x++)
            {
                if (ray->map.tiles[x][y].doorOpen > 0 && ray->map.tiles[x][y].doorOpen < TO_FX(1))
                {
                    ray->map.tiles[x][y].doorOpen++;
                }
            }
        }
    }

    // TODO make a movement vector
    int16_t movementAngle = -1;

    // B button strafes, which may lock on an enemy
    if ((ray->btnState & PB_B) && !(prevBtnState & PB_B))
    {
        // Set strafe to true
        ray->isStrafing = true;
    }
    else if (!(ray->btnState & PB_B) && (prevBtnState & PB_B))
    {
        // Set strafe to false
        ray->isStrafing = false;
    }

    // Strafing is either locked or unlocked
    if (ray->isStrafing)
    {
        if (centeredSprite)
        {
            // Adjust position to always center on the locked target object
            int32_t newAngle = cordicAtan2(centeredSprite->posX - ray->posX, ray->posY - centeredSprite->posY);
            setPlayerAngle(TO_FX(newAngle));
        }

        // Strafe right
        if (ray->btnState & PB_RIGHT)
        {
            // TODO adjust direction vector
        }

        // Strafe left
        if (ray->btnState & PB_LEFT)
        {
            // TODO adjust direction vector
        }
    }
    else
    {
        // Rotate right, in place
        if (ray->btnState & PB_RIGHT)
        {
            q24_8 newAngle = ADD_FX(ray->dirAngle, TO_FX(5));
            if (newAngle >= TO_FX(360))
            {
                newAngle -= TO_FX(360);
            }
            setPlayerAngle(newAngle);
        }

        // Rotate left, in place
        if (ray->btnState & PB_LEFT)
        {
            q24_8 newAngle = SUB_FX(ray->dirAngle, TO_FX(5));
            if (newAngle < TO_FX(0))
            {
                newAngle += TO_FX(360);
            }
            setPlayerAngle(newAngle);
        }
    }

    // Boundary checks are longer than the move dist to not get right up on the wall
    q24_8 boundaryCheckX = ray->dirX / 3;
    q24_8 boundaryCheckY = ray->dirY / 3;
    // Find move distances
    // TODO scale with elapsed time
    q24_8 deltaX = (ray->dirX / 6);
    q24_8 deltaY = (ray->dirY / 6);

    // Move forwards if no wall in front of you
    // TODO adjust movement vector, refactor the actual movement afterwards
    if (ray->btnState & PB_UP)
    {
        if (isPassableCell(&ray->map.tiles[FROM_FX(ray->posX + boundaryCheckX)][FROM_FX(ray->posY)]))
        {
            ray->posX += deltaX;
        }

        if (isPassableCell(&ray->map.tiles[FROM_FX(ray->posX)][FROM_FX(ray->posY + boundaryCheckY)]))
        {
            ray->posY += deltaY;
        }
    }

    // move backwards if no wall behind you
    // TODO adjust movement vector, refactor the actual movement afterwards
    if (ray->btnState & PB_DOWN)
    {
        if (isPassableCell(&ray->map.tiles[FROM_FX(ray->posX - boundaryCheckX)][FROM_FX(ray->posY)]))
        {
            ray->posX -= deltaX;
        }

        if (isPassableCell(&ray->map.tiles[FROM_FX(ray->posX)][FROM_FX(ray->posY - boundaryCheckY)]))
        {
            ray->posY -= deltaY;
        }
    }

    if ((ray->btnState & PB_A) && !(prevBtnState & PB_A))
    {
        // TODO shoot
        for (uint16_t newIdx = 0; newIdx < MAX_RAY_OBJS; newIdx++)
        {
            if (-1 == ray->objs[newIdx].id)
            {
                ray->objs[newIdx].sprite = &ray->texBullet;
                ray->objs[newIdx].dist   = 0;
                ray->objs[newIdx].posX   = ray->posX + ray->dirX / 2;
                ray->objs[newIdx].posY   = ray->posY + ray->dirY / 2;
                ray->objs[newIdx].velX   = ray->dirX;
                ray->objs[newIdx].velY   = ray->dirY;
                ray->objs[newIdx].radius = DIV_FX(TO_FX(ray->texBullet.w), TO_FX(64));
                ray->objs[newIdx].type   = OBJ_BULLET;
                ray->objs[newIdx].id     = 0;
                break;
            }
        }
    }
}

/**
 * @brief Helper function to set the angle the player is facing and associated camera variables
 *
 * @param angle The angle the player is facing. Must be in the range [0, 359]. 0 is north
 */
void setPlayerAngle(q24_8 angle)
{
    // The angle the player is facing
    ray->dirAngle = angle;

    // Compute cartesian direction from angular direction
    // trig functions are already << 10, so / 4 to get to << 8
    ray->dirY = -getCos1024(FROM_FX(ray->dirAngle)) / 4;
    ray->dirX = getSin1024(FROM_FX(ray->dirAngle)) / 4;

    // the 2d rayCaster version of camera plane, orthogonal to the direction vector and scaled to 2/3
    ray->planeX = MUL_FX(-((1 << FRAC_BITS) * 2) / 3, ray->dirY);
    ray->planeY = MUL_FX(((1 << FRAC_BITS) * 2) / 3, ray->dirX);
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
    castFloorCeiling(ray, y, y + h);
}

/**
 * @brief Check if a cell is currently passable
 *
 * @param cell The cell type to check
 * @return true if the cell can be passed through, false if it cannot
 */
static bool isPassableCell(rayMapCell_t* cell)
{
    switch (cell->type)
    {
        case BG_WALL:
        {
            // Never pass through walls
            return false;
        }
        case BG_DOOR:
        {
            // Only pass through open doors
            return (TO_FX(1) == cell->doorOpen);
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
        case OBJ_BULLET:
        default:
        {
            // Always pass through everything else
            return true;
        }
    }
}
