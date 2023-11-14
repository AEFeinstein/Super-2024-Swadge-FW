//==============================================================================
// Includes
//==============================================================================

#include "hdw-btn.h"
#include "touchUtils.h"

#include "ray_player.h"
#include "ray_object.h"
#include "ray_map.h"
#include "ray_pause.h"
#include "ray_script.h"
#include "ray_death_screen.h"
#include "ray_enemy.h"
#include "ray_dialog.h"

//==============================================================================
// Const data
//==============================================================================

/** @brief random dialog when a missile expansion is acquired */
const char* const missilePickupDialog[] = {
    "Hey, you can hold more missiles now! Where? ...You don't want to know!",
    "Hey, this wasn't the critical path, but at least you can express your frustration with "
    "these additional missiles!",
    "Watch, these five missiles are going to come in SO handy, just you wait.",
    "Look at you, overachiever! Have some missiles for being such a good completionist.",
    "Wow, who just leaves a bunch of missiles lying around? This is an all-ages event, come on.",
    "Did you know that missiles go through enemy shields? Bet you thought we made them have "
    "limited ammo for nothing, huh!",
    "Why yes, this is a missile expansion! Mazel Tov!",
};

/** @brief special dialog for one missile */
const char* missileSwimDialog = "All that swimming just for a missile upgrade? LAME.";

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the player
 *
 * @param ray The entire game state
 * @return true if the player was initialized from scratch, false if loaded from NVM
 */
bool initializePlayer(ray_t* ray)
{
    bool initFromScratch = false;
    size_t len           = sizeof(ray->p);
    if (!readNvsBlob(RAY_NVS_KEY, &ray->p, &len))
    {
        initFromScratch = true;

        // Start at map 0, loaded later
        ray->p.mapId          = 0;
        ray->p.mapsVisited[0] = true;

        // ray->p.posX and ray->p.posY (starting position) are set by the map

        // Set the direction
        ray->p.dirX = TO_FX(0);
        ray->p.dirY = -TO_FX(1);

        // Zero the entire inventory
        memset(&ray->p.i, 0, sizeof(ray->p.i));
        // Set all pickup IDs to -1
        for (int32_t mapIdx = 0; mapIdx < NUM_MAPS; mapIdx++)
        {
            for (int32_t pIdx = 0; pIdx < MISSILE_UPGRADES_PER_MAP; pIdx++)
            {
                ray->p.i.missilesPickUps[mapIdx][pIdx] = -1;
            }
            for (int32_t pIdx = 0; pIdx < E_TANKS_PER_MAP; pIdx++)
            {
                ray->p.i.healthPickUps[mapIdx][pIdx] = -1;
            }
        }
        // Set initial health
        ray->p.i.maxHealth = GAME_START_HEALTH;
        ray->p.i.health    = GAME_START_HEALTH;

        // Set damage multiplier to 1;
        ray->p.i.damageMult = 1;
    }
    else
    {
        // If there is no loadout but the player has the beam
        if ((LO_NONE == ray->p.loadout) && (ray->p.i.beamLoadOut))
        {
            // Set the beam loadout
            ray->p.loadout = LO_NORMAL;
        }
    }

    // the 2d rayCaster version of camera plane, orthogonal to the direction vector and scaled to 2/3
    ray->planeX = -MUL_FX(TO_FX(2) / 3, ray->p.dirY);
    ray->planeY = MUL_FX(TO_FX(2) / 3, ray->p.dirX);

    // Always start with at most ten missiles to not get locked behind doors
    if (ray->p.i.missileLoadOut)
    {
        if (10 > ray->p.i.numMissiles)
        {
            if (10 > ray->p.i.maxNumMissiles)
            {
                ray->p.i.numMissiles = ray->p.i.maxNumMissiles;
            }
            else
            {
                ray->p.i.maxNumMissiles = 10;
            }
        }
    }

    // Set the next loadout to current, to not swap
    ray->nextLoadout        = ray->p.loadout;
    ray->loadoutChangeTimer = 0;
    ray->forceLoadoutSwap   = 0;

    // Always reload with full health
    ray->p.i.health = ray->p.i.maxHealth;

    return initFromScratch;
}

/**
 * @brief Save the entire player state to NVM
 *
 * @param ray The entire game state
 */
void raySavePlayer(ray_t* ray)
{
    writeNvsBlob(RAY_NVS_KEY, &(ray->p), sizeof(ray->p));
}

/**
 * @brief Save the tiles the player has visited in this map
 *
 * @param ray The entire game state
 */
void raySaveVisitedTiles(ray_t* ray)
{
    writeNvsBlob(RAY_NVS_VISITED_KEYS[ray->p.mapId], ray->map.visitedTiles,
                 sizeof(rayTileState_t) * ray->map.w * ray->map.h);
}

/**
 * @brief Check button inputs for the player. This will move the player and shoot bullets
 *
 * @param ray The entire game state
 * @param centeredEnemy The enemy currently centered in the view, may be NULL
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayPlayerCheckButtons(ray_t* ray, rayObjCommon_t* centeredEnemy, uint32_t elapsedUs)
{
    // For convenience
    q24_8 pPosX = ray->p.posX;
    q24_8 pPosY = ray->p.posY;
    q24_8 pDirX = ray->p.dirX;
    q24_8 pDirY = ray->p.dirY;

    // Check all queued button events
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the current button state
        ray->btnState = evt.state;

        // The start button enters the pause menu
        if (PB_START == evt.button && evt.down)
        {
            // Show the pause menu
            rayShowPause(ray);
            // Cancel any charges
            ray->chargeTimer        = 0;
            ray->loadoutChangeTimer = 0;
            ray->forceLoadoutSwap   = false;
            return;
        }
        // The B button toggles strafing
        else if (PB_B == evt.button)
        {
            if (evt.down)
            {
                // Set strafe to true
                ray->isStrafing = true;
                // If there is a centered sprite
                if (centeredEnemy)
                {
                    // Lock onto it
                    ray->targetedObj = centeredEnemy;
                }
            }
            else
            {
                // Set strafe to false
                ray->isStrafing  = false;
                ray->targetedObj = NULL;
            }
        }
        // The A button shoots. Make sure there is a gun
        else if ((LO_NONE != ray->p.loadout) && (PB_A == evt.button))
        {
            // What, if any, bullet to fire
            rayMapCellType_t bullet = EMPTY;

            if (evt.down)
            {
                if (0 >= ray->playerShotCooldown)
                {
                    // A was pressed
                    // Check ammo for the missile loadout
                    if (LO_MISSILE == ray->p.loadout)
                    {
                        if (0 < ray->p.i.numMissiles)
                        {
                            // Decrement missile count
                            ray->p.i.numMissiles--;
                            // Fire a missile
                            bullet = OBJ_BULLET_MISSILE;
                        }
                    }
                    else
                    {
                        // Start charging if applicable
                        if (LO_NORMAL == ray->p.loadout && ray->p.i.chargePowerUp)
                        {
                            // Start charging
                            ray->chargeTimer = 1;
                        }

                        // Fire according to loadout
                        const rayMapCellType_t bulletMap[NUM_LOADOUTS] = {
                            EMPTY,
                            OBJ_BULLET_NORMAL,  ///< Normal loadout
                            OBJ_BULLET_MISSILE, ///< Missile loadout
                            OBJ_BULLET_ICE,     ///< Ice beam loadout
                            OBJ_BULLET_XRAY     ///< X-Ray loadout
                        };
                        bullet = bulletMap[ray->p.loadout];
                    }
                }
            }
            else
            {
                // A was released, check if the beam is charged
                if (ray->chargeTimer >= CHARGE_TIME_US)
                {
                    // Shoot charge beam
                    bullet = OBJ_BULLET_CHARGE;
                }
                ray->chargeTimer = 0;
            }

            // If there is a bullet to fire
            if (EMPTY != bullet)
            {
                // Fire a shot
                rayCreateBullet(ray, bullet, pPosX, pPosY, pDirX, pDirY, true);

                // Play SFX depending on bullet
                switch (bullet)
                {
                    case OBJ_BULLET_CHARGE:
                    {
                        ray->playerShotCooldown = 240000;
                        bzrPlaySfx(&ray->sfx_p_charge, BZR_RIGHT);
                        break;
                    }
                    case OBJ_BULLET_MISSILE:
                    {
                        ray->playerShotCooldown = 240000;
                        bzrPlaySfx(&ray->sfx_p_missile, BZR_RIGHT);
                        break;
                    }
                    case OBJ_BULLET_ICE:
                    {
                        ray->playerShotCooldown = 480000;
                        bzrPlaySfx(&ray->sfx_p_ice, BZR_RIGHT);
                        break;
                    }
                    case OBJ_BULLET_XRAY:
                    {
                        ray->playerShotCooldown = 240000;
                        bzrPlaySfx(&ray->sfx_p_xray, BZR_RIGHT);
                        break;
                    }
                    case OBJ_BULLET_NORMAL:
                    default:
                    {
                        ray->playerShotCooldown = 120000;
                        bzrPlaySfx(&ray->sfx_p_shoot, BZR_RIGHT);
                        break;
                    }
                }
            }
        }
    }

    // Count down shot cooldown
    if (0 < ray->playerShotCooldown)
    {
        ray->playerShotCooldown -= elapsedUs;
    }

    // If charging the beam
    if (0 < ray->chargeTimer && ray->chargeTimer <= CHARGE_TIME_US)
    {
        // Accumulate the timer
        ray->chargeTimer += elapsedUs;
    }

    // If the player is in water without the water suit
    bool isInWater = (!ray->p.i.waterSuit) && (BG_FLOOR_WATER == ray->map.tiles[FROM_FX(pPosX)][FROM_FX(pPosY)].type);

    // Find move distances
    q24_8 deltaX = 0;
    q24_8 deltaY = 0;

    // Strafing is either locked or unlocked
    if (ray->isStrafing)
    {
        if (NULL != ray->targetedObj)
        {
            // lock on the target before moving
            pDirX = ray->targetedObj->posX - pPosX;
            pDirY = ray->targetedObj->posY - pPosY;
            fastNormVec(&pDirX, &pDirY);
        }

        if (ray->btnState & PB_RIGHT)
        {
            // Strafe right
            deltaX -= pDirY;
            deltaY += pDirX;
        }
        else if (ray->btnState & PB_LEFT)
        {
            // Strafe left
            deltaX += pDirY;
            deltaY -= pDirX;
        }
    }
    else
    {
        if (ray->btnState & (PB_RIGHT | PB_LEFT))
        {
            // Assume rightward rotation, 1 degree every 8000uS
            int32_t rotateDeg = 0;
            ray->pRotationTimer -= elapsedUs;
            while (0 >= ray->pRotationTimer)
            {
                ray->pRotationTimer += 8000;
                rotateDeg++;
            }

            if (0 != rotateDeg)
            {
                if (ray->btnState & PB_LEFT)
                {
                    // Rotate left, reverse direction
                    rotateDeg = 360 - rotateDeg;
                }

                // Do trig functions, only once
                int32_t sinVal = getSin1024(rotateDeg);
                int32_t cosVal = getCos1024(rotateDeg);
                // Find the rotated X and Y vectors
                q24_8 newX = (pDirX * cosVal) - (pDirY * sinVal);
                q24_8 newY = (pDirX * sinVal) + (pDirY * cosVal);
                // Normalize the vector
                fastNormVec(&newX, &newY);

                // Save new direction vector
                ray->p.dirX = newX;
                ray->p.dirY = newY;

                // Recompute the camera plane, orthogonal to the direction vector and scaled to 2/3
                ray->planeX = -MUL_FX(TO_FX(2) / 3, ray->p.dirY);
                ray->planeY = MUL_FX(TO_FX(2) / 3, ray->p.dirX);

                // Also update the local copy
                pDirX = newX;
                pDirY = newY;
            }
        }
    }

    // If the up button is held
    if (ray->btnState & PB_UP)
    {
        // Move forward
        deltaX += pDirX;
        deltaY += pDirY;
    }
    // Else if the down button is held
    else if (ray->btnState & PB_DOWN)
    {
        // Move backwards
        deltaX -= pDirX;
        deltaY -= pDirY;
    }

    // If there is movement
    if (deltaX || deltaY)
    {
        // Normalize deltaX and deltaY before scaling with elapsedUs
        fastNormVec(&deltaX, &deltaY);

        // Save normalized vector before scaling for boundary checks later
        q24_8 normX = deltaX;
        q24_8 normY = deltaY;

        // Should move 1/6 units every 40000uS
        deltaX = (int32_t)(deltaX * elapsedUs) / (int32_t)(40000 * 6);
        deltaY = (int32_t)(deltaY * elapsedUs) / (int32_t)(40000 * 6);

        // If the player is in water
        if (isInWater)
        {
            // Slow down movement by a 8x
            deltaX /= 8;
            deltaY /= 8;
        }

        // Boundary checks are longer than the move dist to not get right up on the wall
        q24_8 boundaryCheckX = (2 * normX) / 3;
        q24_8 boundaryCheckY = (2 * normY) / 3;

        // Save the old cell to check for crossing cell boundaries
        int16_t oldCellX = FROM_FX(pPosX);
        int16_t oldCellY = FROM_FX(pPosY);

        // Move forwards if no wall in front of you
        if (isPassableCell(&ray->map.tiles[FROM_FX(pPosX + boundaryCheckX)][FROM_FX(pPosY)]))
        {
            ray->p.posX += deltaX;
            // Update local copy
            pPosX = ray->p.posX;
        }

        if (isPassableCell(&ray->map.tiles[FROM_FX(pPosX)][FROM_FX(pPosY + boundaryCheckY)]))
        {
            ray->p.posY += deltaY;
            // Update local copy
            pPosY = ray->p.posY;
        }

        // Get the new cell to check for crossing cell boundaries
        int16_t newCellX = FROM_FX(ray->p.posX);
        int16_t newCellY = FROM_FX(ray->p.posY);

        // If the cell changed
        if (oldCellX != newCellX || oldCellY != newCellY)
        {
            // Mark it on the map
            markTileVisited(&ray->map, newCellX, newCellY);

            // Check scripts when entering cells
            checkScriptEnter(ray, newCellX, newCellY);
        }
    }

    // Finally, if there is a targeted object, orient towards it
    if (ray->targetedObj)
    {
        // Re-lock on the target
        pDirX = ray->targetedObj->posX - pPosX;
        pDirY = ray->targetedObj->posY - pPosY;
        fastNormVec(&pDirX, &pDirY);

        // Set the player's direction
        ray->p.dirX = pDirX;
        ray->p.dirY = pDirY;

        // Recompute the 2d rayCaster version of camera plane, orthogonal to the direction vector and scaled to 2/3
        ray->planeX = -MUL_FX(TO_FX(2) / 3, ray->p.dirY);
        ray->planeY = MUL_FX(TO_FX(2) / 3, ray->p.dirX);
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
                rayLoadout_t nextLoadout = ray->p.loadout;
                if ((tj & TB_UP) && (ray->p.i.beamLoadOut))
                {
                    nextLoadout = LO_NORMAL;
                }
                else if ((tj & TB_RIGHT) && (ray->p.i.missileLoadOut))
                {
                    nextLoadout = LO_MISSILE;
                }
                else if ((tj & TB_LEFT) && (ray->p.i.iceLoadOut))
                {
                    nextLoadout = LO_ICE;
                }
                else if ((tj & TB_DOWN) && (ray->p.i.xrayLoadOut))
                {
                    nextLoadout = LO_XRAY;
                }

                // If a new loadout was touched
                if (ray->p.loadout != nextLoadout)
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
        if ((ray->nextLoadout != ray->p.loadout) && !ray->forceLoadoutSwap)
        {
            // Reset the timer to bring the gun up and set the next loadout to the current one
            ray->loadoutChangeTimer = LOADOUT_TIMER_US - ray->loadoutChangeTimer;
            ray->nextLoadout        = ray->p.loadout;
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
            if (ray->p.loadout != ray->nextLoadout)
            {
                // If swapping to or from XRAY, also swap hidden enemy sprites
                if (LO_XRAY == ray->nextLoadout)
                {
                    switchEnemiesToXray(ray, true);
                }
                else if (LO_XRAY == ray->p.loadout)
                {
                    switchEnemiesToXray(ray, false);
                }

                // Swap the loadout
                ray->p.loadout = ray->nextLoadout;
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
 * @param item The item that was touched
 * @param mapId The current map ID, used to track non-unique persistent pick-ups
 */
void rayPlayerTouchItem(ray_t* ray, rayObjCommon_t* item, int32_t mapId)
{
    rayMapCellType_t type = item->type;
    int32_t itemId        = item->id;

    // Assume saving after picking up an item
    bool saveAfterObtain      = true;
    rayInventory_t* inventory = &ray->p.i;
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
            if (LO_NORMAL != ray->p.loadout)
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
            for (int32_t idx = 0; idx < MISSILE_UPGRADES_PER_MAP; idx++)
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
                    inventory->numMissiles += 5;
                    inventory->maxNumMissiles += 5;
                    // Cap at 99 missiles
                    if (inventory->maxNumMissiles > 99)
                    {
                        inventory->maxNumMissiles = 99;
                    }

                    // Save the coordinates
                    inventory->missilesPickUps[mapId][idx] = itemId;

                    // World 3, Missile ID 12 gets special dialog
                    if (3 == ray->p.mapId && 12 == item->id)
                    {
                        rayShowDialog(ray, missileSwimDialog, item->sprite);
                    }
                    else
                    {
                        // Show random dialog
                        int32_t dialogIdx = esp_random() % ARRAY_SIZE(missilePickupDialog);
                        rayShowDialog(ray, missilePickupDialog[dialogIdx], item->sprite);
                    }
                    break;
                }
            }
            break;
        }
        case OBJ_ITEM_ENERGY_TANK:
        {
            // Find a slot to save this health pickup
            for (int32_t idx = 0; idx < E_TANKS_PER_MAP; idx++)
            {
                // The ID of an item can't be -1, so this is free
                if (-1 == inventory->healthPickUps[mapId][idx])
                {
                    // Add max health
                    inventory->maxHealth += HEALTH_PER_E_TANK;
                    // Cap the max health
                    if (inventory->maxHealth > MAX_HEALTH_EVER)
                    {
                        inventory->maxHealth = MAX_HEALTH_EVER;
                    }

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

            // Check if all artifacts have been collected
            bool collected = true;
            for (int16_t aIdx = 0; aIdx < ARRAY_SIZE(inventory->artifacts); aIdx++)
            {
                if (!inventory->artifacts[aIdx])
                {
                    collected = false;
                    break;
                }
            }

            // If all were collected
            if (collected)
            {
                // Increase damage output by 2
                inventory->damageMult = 2;
            }
            break;
        }
        case OBJ_ITEM_PICKUP_ENERGY:
        {
            // Transient, add (GAME_START_HEALTH / 2) health, not going over the max
            inventory->health = MIN(inventory->health + (GAME_START_HEALTH / 2), inventory->maxHealth);
            // Play SFX
            bzrPlaySfx(&ray->sfx_health, BZR_RIGHT);
            // Don't save after energy
            saveAfterObtain = false;
            break;
        }
        case OBJ_ITEM_PICKUP_MISSILE:
        {
            if (ray->p.i.missileLoadOut)
            {
                // Transient, add 5 missiles, not going over the max
                inventory->numMissiles = MIN(inventory->numMissiles + 5, inventory->maxNumMissiles);
            }
            // Play SFX
            bzrPlaySfx(&ray->sfx_health, BZR_RIGHT);
            // Don't save after missile ammo
            saveAfterObtain = false;
            break;
        }
        case OBJ_ITEM_KEY_A:
        case OBJ_ITEM_KEY_B:
        case OBJ_ITEM_KEY_C:
        {
            // Pick up a key
            inventory->keys[ray->p.mapId][type - OBJ_ITEM_KEY_A] = KEY;
            break;
        }
        default:
        {
            // Don't care about other types
            saveAfterObtain = false;
            break;
        }
    }

    // If a notable item was obtained
    if (saveAfterObtain)
    {
        // Autosave
        raySavePlayer(ray);
        raySaveVisitedTiles(ray);
        // Play SFX
        bzrPlaySfx(&ray->sfx_item_get, BZR_RIGHT);
    }
}

/**
 * @brief Check if a player should take damage for standing in lava
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayPlayerCheckFloorEffect(ray_t* ray, uint32_t elapsedUs)
{
    // If the player is in lava without the lava suit
    if ((!ray->p.i.lavaSuit) && (BG_FLOOR_LAVA == ray->map.tiles[FROM_FX(ray->p.posX)][FROM_FX(ray->p.posY)].type))
    {
        // Run a timer to take lava damage
        ray->floorEffectTimer += elapsedUs;
        if (ray->floorEffectTimer <= US_PER_FLOOR_EFFECT)
        {
            ray->floorEffectTimer -= US_PER_FLOOR_EFFECT;
            rayPlayerDecrementHealth(ray, 1);
        }

        // If the player is entering lava
        if (false == ray->playerInLava)
        {
            ray->playerInLava = true;
            // Start looping SFX
            // ray->sfx_lava_dmg.shouldLoop = true;
            // bzrPlaySfx(&ray->sfx_lava_dmg, BZR_RIGHT);
        }
    }
    else if (BG_FLOOR_HEAL == ray->map.tiles[FROM_FX(ray->p.posX)][FROM_FX(ray->p.posY)].type)
    {
        // Run a timer to heal
        ray->floorEffectTimer += elapsedUs;
        if (ray->floorEffectTimer <= US_PER_FLOOR_EFFECT)
        {
            ray->floorEffectTimer -= US_PER_FLOOR_EFFECT;
            rayPlayerDecrementHealth(ray, -1);
        }
    }
    else if (true == ray->playerInLava)
    {
        ray->playerInLava = false;
        // Stop looping SFX
        // ray->sfx_lava_dmg.shouldLoop = true;
    }
}

/**
 * @brief Decrement player health and check for death
 *
 * @param ray The entire game state
 * @param health The amount of health to decrement
 */
void rayPlayerDecrementHealth(ray_t* ray, int32_t health)
{
    // Decrement health
    ray->p.i.health -= health;

    if (health > 0)
    {
        // Play SFX
        bzrPlaySfx(&ray->sfx_p_damage, BZR_RIGHT);
    }

    // Make LEDs red
    ray->ledHue = 0;

    // Check for death
    if (0 >= ray->p.i.health)
    {
        // load the last save
        rayStartGame();

        // Show the death screen
        rayShowDeathScreen(ray);
    }
    else if (ray->p.i.health > ray->p.i.maxHealth)
    {
        // Never go over the max health
        ray->p.i.health = ray->p.i.maxHealth;
    }
}
