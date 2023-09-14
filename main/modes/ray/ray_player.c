//==============================================================================
// Includes
//==============================================================================

#include "hdw-btn.h"
#include "touchUtils.h"

#include "ray_player.h"
#include "ray_object.h"
#include "ray_map.h"

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
    // Set the direction
    ray->dirX = TO_FX(0);
    ray->dirY = -TO_FX(1);
    // the 2d rayCaster version of camera plane, orthogonal to the direction vector and scaled to 2/3
    ray->planeX = -MUL_FX(TO_FX(2) / 3, ray->dirY);
    ray->planeY = MUL_FX(TO_FX(2) / 3, ray->dirX);
    // Set the head-bob to centered
    ray->posZ = TO_FX(0);

    // Empty out the inventory
    // TODO only do this on first boot-up
    memset(&ray->inventory, 0, sizeof(ray->inventory));
    for (int32_t mapIdx = 0; mapIdx < NUM_MAPS; mapIdx++)
    {
        for (int32_t pIdx = 0; pIdx < NUM_PICKUPS_PER_MAP; pIdx++)
        {
            ray->inventory.healthPickUps[mapIdx][pIdx]   = -1;
            ray->inventory.missilesPickUps[mapIdx][pIdx] = -1;
        }
    }
}

/**
 * @brief Check button inputs for the player. This will move the player and shoot bullets
 *
 * @param ray The entire game state
 * @param centeredSprite The sprite currently centered in the view
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayPlayerCheckButtons(ray_t* ray, rayObjCommon_t* centeredSprite, uint32_t elapsedUs)
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
        // If there is a centered sprite
        if (centeredSprite)
        {
            // Lock onto it
            ray->targetedObj = centeredSprite;
        }
    }
    else if (!(ray->btnState & PB_B) && (prevBtnState & PB_B))
    {
        // Set strafe to false
        ray->isStrafing  = false;
        ray->targetedObj = NULL;
    }

    // Strafing is either locked or unlocked
    if (ray->isStrafing)
    {
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
        // Assume no rotation
        int32_t rotateDeg = 0;

        if (ray->btnState & PB_RIGHT)
        {
            // Rotate right
            // TODO scale with elapsed time
            rotateDeg = 5;
        }
        else if (ray->btnState & PB_LEFT)
        {
            // Rotate left
            // TODO scale with elapsed time
            rotateDeg = 355;
        }

        // If we should rotate
        if (rotateDeg)
        {
            // Do trig functions, only once
            int32_t sinVal = getSin1024(rotateDeg);
            int32_t cosVal = getCos1024(rotateDeg);
            // Find the rotated X and Y vectors
            q24_8 newX = (ray->dirX * cosVal) - (ray->dirY * sinVal);
            q24_8 newY = (ray->dirX * sinVal) + (ray->dirY * cosVal);
            ray->dirX  = newX;
            ray->dirY  = newY;
            // Normalize the vector
            fastNormVec(&ray->dirX, &ray->dirY);

            // Recompute the camera plane, orthogonal to the direction vector and scaled to 2/3
            ray->planeX = -MUL_FX(TO_FX(2) / 3, ray->dirY);
            ray->planeY = MUL_FX(TO_FX(2) / 3, ray->dirX);
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

    // After moving position, recompute direction to targeted object
    if (ray->isStrafing && ray->targetedObj)
    {
        // Re-lock on the target after moving
        ray->dirX = ray->targetedObj->posX - ray->posX;
        ray->dirY = ray->targetedObj->posY - ray->posY;
        fastNormVec(&ray->dirX, &ray->dirY);

        // Recompute the 2d rayCaster version of camera plane, orthogonal to the direction vector and scaled to 2/3
        ray->planeX = -MUL_FX(TO_FX(2) / 3, ray->dirY);
        ray->planeY = MUL_FX(TO_FX(2) / 3, ray->dirX);
    }

    // If the A button is pressed
    if ((ray->btnState & PB_A) && !(prevBtnState & PB_A) && (LO_NONE != ray->loadout))
    {
        // Easy map for loadout to bullet type
        const rayMapCellType_t bulletMap[NUM_LOADOUTS] = {
            EMPTY,
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
                rayLoadout_t nextLoadout = ray->loadout;
                if ((tj & TB_UP) && (ray->inventory.beamLoadOut))
                {
                    nextLoadout = LO_NORMAL;
                }
                else if ((tj & TB_RIGHT) && (ray->inventory.missileLoadOut))
                {
                    nextLoadout = LO_MISSILE;
                }
                else if ((tj & TB_LEFT) && (ray->inventory.iceLoadOut))
                {
                    nextLoadout = LO_ICE;
                }
                else if ((tj & TB_DOWN) && (ray->inventory.xrayLoadOut))
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
        if ((ray->nextLoadout != ray->loadout) && !ray->forceLoadoutSwap)
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
                ray->forceLoadoutSwap   = false;
            }
        }
    }
}

/**
 * @brief This handles what happens when a player touches an item
 *
 * @param ray The whole game state
 * @param type The type of item touched
 * @param mapId The current map ID, used to track non-unique persistent pick-ups
 * @param itemId The item's ID
 */
void rayPlayerTouchItem(ray_t* ray, rayMapCellType_t type, int32_t mapId, int32_t itemId)
{
    rayInventory_t* inventory = &ray->inventory;
    switch (type)
    {
        case OBJ_ITEM_BEAM:
        {
            inventory->beamLoadOut = true;
            // Switch to the normal loadout
            ray->loadoutChangeTimer = LOADOUT_TIMER_US;
            ray->nextLoadout        = LO_NORMAL;
            ray->forceLoadoutSwap   = true;
            break;
        }
        case OBJ_ITEM_CHARGE_BEAM:
        {
            inventory->chargePowerUp = true;
            if (LO_NORMAL != ray->loadout)
            {
                // Switch to the normal loadout
                ray->loadoutChangeTimer = LOADOUT_TIMER_US;
                ray->nextLoadout        = LO_NORMAL;
                ray->forceLoadoutSwap   = true;
            }
            break;
        }
        case OBJ_ITEM_ICE:
        {
            inventory->iceLoadOut = true;
            // Switch to the ice loadout
            ray->loadoutChangeTimer = LOADOUT_TIMER_US;
            ray->nextLoadout        = LO_ICE;
            ray->forceLoadoutSwap   = true;
            break;
        }
        case OBJ_ITEM_XRAY:
        {
            inventory->xrayLoadOut = true;
            // Switch to the xray loadout
            ray->loadoutChangeTimer = LOADOUT_TIMER_US;
            ray->nextLoadout        = LO_XRAY;
            ray->forceLoadoutSwap   = true;
            break;
        }
        case OBJ_ITEM_SUIT_WATER:
        {
            inventory->waterSuit = true;
            break;
        }
        case OBJ_ITEM_SUIT_LAVA:
        {
            inventory->lavaSuit = true;
            break;
        }
        case OBJ_ITEM_MISSILE:
        {
            // Find a slot to save this missile pickup
            for (int32_t idx = 0; idx < NUM_PICKUPS_PER_MAP; idx++)
            {
                // The ID of an item can't be -1, so this is free
                if (-1 == inventory->missilesPickUps[mapId][idx])
                {
                    if (!inventory->missileLoadOut)
                    {
                        // Picking up any missiles enables the loadout
                        inventory->missileLoadOut = true;
                        // Switch to the missile loadout
                        ray->loadoutChangeTimer = LOADOUT_TIMER_US;
                        ray->nextLoadout        = LO_MISSILE;
                        ray->forceLoadoutSwap   = true;
                    }
                    // Add five missiles
                    inventory->maxNumMissiles += 5;

                    // Save the coordinates
                    inventory->missilesPickUps[mapId][idx] = itemId;
                    break;
                }
            }
            break;
        }
        case OBJ_ITEM_ENERGY_TANK:
        {
            // Find a slot to save this health pickup
            for (int32_t idx = 0; idx < NUM_PICKUPS_PER_MAP; idx++)
            {
                // The ID of an item can't be -1, so this is free
                if (-1 == inventory->healthPickUps[mapId][idx])
                {
                    // Add 100 health
                    inventory->maxHealth += 100;
                    // Reset health
                    inventory->health = inventory->maxHealth;

                    // Save the coordinates
                    inventory->healthPickUps[mapId][idx] = itemId;
                    break;
                }
            }
            break;
        }
        case OBJ_ITEM_ARTIFACT:
        {
            inventory->artifacts[mapId] = true;
            break;
        }
        case OBJ_ITEM_PICKUP_ENERGY:
        {
            // Transient, add 20 health, not going over the max
            inventory->health = MIN(inventory->health + 20, inventory->maxHealth);
            break;
        }
        case OBJ_ITEM_PICKUP_MISSILE:
        {
            if (ray->inventory.missileLoadOut)
            {
                // Transient, add 5 missiles, not going over the max
                inventory->numMissiles = MIN(inventory->numMissiles + 5, inventory->maxNumMissiles);
            }
            break;
        }
        case OBJ_ITEM_KEY:
        {
            // TODO implement keys?
            break;
        }
        default:
        {
            // Don't care about other types
            break;
        }
    }
}