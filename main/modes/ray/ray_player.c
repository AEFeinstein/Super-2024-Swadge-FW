//==============================================================================
// Includes
//==============================================================================

#include "hdw-btn.h"
#include "touchUtils.h"

#include "ray_player.h"
#include "ray_object.h"
#include "ray_map.h"

//==============================================================================
// Function Prototypes
//==============================================================================

static void setPlayerAngle(ray_t* ray, q24_8 angle);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the player
 *
 * @param ray The entire game state
 */
void initializePlayer(ray_t* ray)
{
    // ray->posX and ray->posY (position) are set by loadRayMap()
    // Set the direction angle to 0
    setPlayerAngle(ray, TO_FX(0));
    // Set the head-bob to centered
    ray->posZ = TO_FX(0);
}

/**
 * @brief Check button inputs for the player. This will move the player and shoot bullets
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayPlayerCheckButtons(ray_t* ray, uint32_t elapsedUs)
{
    // Check all queued button events
    uint32_t prevBtnState = ray->btnState;
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        ray->btnState = evt.state;
    }

    // Find move distances
    q24_8 deltaX = 0;
    q24_8 deltaY = 0;

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
        if (ray->targetedObj)
        {
            // Adjust position to always center on the locked target object
            int32_t newAngle = cordicAtan2(ray->targetedObj->posX - ray->posX, ray->posY - ray->targetedObj->posY);
            setPlayerAngle(ray, TO_FX(newAngle));
            // This style of strafe (adjust angle, move tangentially) makes the player slowly spiral outward. Oh well.
        }

        if (ray->btnState & PB_RIGHT)
        {
            // Strafe right
            // TODO scale with elapsed time
            deltaX -= (ray->dirY / 6);
            deltaY += (ray->dirX / 6);
        }
        else if (ray->btnState & PB_LEFT)
        {
            // Strafe left
            // TODO scale with elapsed time
            deltaX += (ray->dirY / 6);
            deltaY -= (ray->dirX / 6);
        }
    }
    else
    {
        // Rotate right, in place
        if (ray->btnState & PB_RIGHT)
        {
            // TODO scale with elapsed time
            q24_8 newAngle = ADD_FX(ray->dirAngle, TO_FX(5));
            if (newAngle >= TO_FX(360))
            {
                newAngle -= TO_FX(360);
            }
            setPlayerAngle(ray, newAngle);
        }

        // Rotate left, in place
        if (ray->btnState & PB_LEFT)
        {
            // TODO scale with elapsed time
            q24_8 newAngle = SUB_FX(ray->dirAngle, TO_FX(5));
            if (newAngle < TO_FX(0))
            {
                newAngle += TO_FX(360);
            }
            setPlayerAngle(ray, newAngle);
        }
    }

    // If the up button is held
    if (ray->btnState & PB_UP)
    {
        // Move forward
        // TODO scale with elapsed time
        deltaX += (ray->dirX / 6);
        deltaY += (ray->dirY / 6);
    }
    // Else if the down button is held
    else if (ray->btnState & PB_DOWN)
    {
        // Move backwards
        // TODO scale with elapsed time
        deltaX -= (ray->dirX / 6);
        deltaY -= (ray->dirY / 6);
    }

    // TODO normalize deltaX and deltaY to something scaled with elapsed time

    // Boundary checks are longer than the move dist to not get right up on the wall
    q24_8 boundaryCheckX = deltaX * 2;
    q24_8 boundaryCheckY = deltaY * 2;

    // Move forwards if no wall in front of you
    if (isPassableCell(&ray->map.tiles[FROM_FX(ray->posX + boundaryCheckX)][FROM_FX(ray->posY)]))
    {
        ray->posX += deltaX;
    }

    if (isPassableCell(&ray->map.tiles[FROM_FX(ray->posX)][FROM_FX(ray->posY + boundaryCheckY)]))
    {
        ray->posY += deltaY;
    }

    // If the A button is pressed
    if ((ray->btnState & PB_A) && !(prevBtnState & PB_A))
    {
        // Easy map for loadout to bullet type
        const rayMapCellType_t bulletMap[NUM_LOADOUTS] = {
            OBJ_BULLET_NORMAL, ///< Normal loadout
            // TODO charge beam unaccounted for
            OBJ_BULLET_MISSILE, ///< Missile loadout
            OBJ_BULLET_ICE,     ///< Ice beam loadout
            OBJ_BULLET_XRAY     ///< X-Ray loadout
        };

        // Fire a shot
        rayCreateBullet(ray, bulletMap[ray->loadout], ray->posX, ray->posY, ray->dirX, ray->dirY, true);
    }
}

/**
 * @brief Check touchpad inputs for the player. This will change the player's loadout
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayPlayerCheckJoystick(ray_t* ray, uint32_t elapsedUs)
{
    // Check if the touch area is touched, and print values if it is
    int32_t phi, r, intensity;
    if (getTouchJoystick(&phi, &r, &intensity))
    {
        // If there isn't a loadout change in progress
        if (ray->loadoutChangeTimer == 0)
        {
            // Get the zones from the touchpad
            touchJoystick_t tj = getTouchJoystickZones(phi, r, true, false);
            if (!(tj & TB_CENTER))
            {
                // Get the loadout touched
                rayLoadout_t nextLoadout = LO_NORMAL;
                if (tj & TB_UP)
                {
                    nextLoadout = LO_NORMAL;
                }
                else if (tj & TB_RIGHT)
                {
                    nextLoadout = LO_MISSILE;
                }
                else if (tj & TB_LEFT)
                {
                    nextLoadout = LO_ICE;
                }
                else if (tj & TB_DOWN)
                {
                    nextLoadout = LO_XRAY;
                }

                // If a new loadout was touched
                if (ray->loadout != nextLoadout)
                {
                    // Start a timer to switch to the next loadout
                    ray->loadoutChangeTimer = LOADOUT_TIMER_US;
                    ray->nextLoadout        = nextLoadout;
                }
            }
        }
    }
    else
    {
        // Button Not touched. If this was during a loadout change, cancel it
        if (ray->nextLoadout != ray->loadout)
        {
            // Reset the timer to bring the gun up and set the next loadout to the current one
            ray->loadoutChangeTimer = LOADOUT_TIMER_US - ray->loadoutChangeTimer;
            ray->nextLoadout        = ray->loadout;
        }
    }

    // If a loadout change is in progress
    if (ray->loadoutChangeTimer)
    {
        // Decrement the timer
        ray->loadoutChangeTimer -= elapsedUs;

        // If the timer elapsed
        if (ray->loadoutChangeTimer <= 0)
        {
            if (ray->loadout != ray->nextLoadout)
            {
                // Swap the loadout
                ray->loadout = ray->nextLoadout;
                // Set the timer for the load in
                ray->loadoutChangeTimer = LOADOUT_TIMER_US;
            }
            else
            {
                // All done swapping
                ray->loadoutChangeTimer = 0;
            }
        }
    }
}

/**
 * @brief Helper function to set the angle the player is facing and associated camera variables
 *
 * @param ray The entire game state
 * @param angle The angle the player is facing. Must be in the range [0, 359]. 0 is north
 */
void setPlayerAngle(ray_t* ray, q24_8 angle)
{
    // The angle the player is facing
    ray->dirAngle = angle;

    // Compute cartesian direction from angular direction
    // trig functions are already << 10, so / 4 to get to << 8
    ray->dirY = -getCos1024(FROM_FX(ray->dirAngle)) / 4;
    ray->dirX = getSin1024(FROM_FX(ray->dirAngle)) / 4;

    // the 2d rayCaster version of camera plane, orthogonal to the direction vector and scaled to 2/3
    ray->planeX = MUL_FX(-TO_FX(2) / 3, ray->dirY);
    ray->planeY = MUL_FX(TO_FX(2) / 3, ray->dirX);
}
