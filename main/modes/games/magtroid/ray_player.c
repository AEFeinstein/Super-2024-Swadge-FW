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

#define SWORD_SWING_TIME  200000
#define SWORD_SWING_ANGLE 120

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

    // Initial touch state is negative
    ray->p.ts.initialTouchPos = -1;
    ray->p.ts.lastTouchPos    = -1;

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
            if (evt.down && ray->p.swordTimerUs <= 0)
            {
                // Start a sword swing
                ray->p.swordAngle = rayGetEightWayAngle(ray->p.dirX, ray->p.dirY);
                ray->p.swordAngle -= 90;
                if (ray->p.swordAngle < 0)
                {
                    ray->p.swordAngle += 360;
                }
                ray->p.swordTimerUs = SWORD_SWING_TIME;
            }
        }
        // The A button shoots. Make sure there is a gun
        else if (PB_A == evt.button)
        {
            if (evt.down)
            {
                // TODO Jump
                printf("JUMP!\n");
            }
        }
    }

    // Find move distances
    q24_8 deltaX = 0;
    q24_8 deltaY = 0;

    // If the up button is held
    if (ray->btnState & PB_UP)
    {
        // Move forward
        deltaY -= 1;
    }
    // Else if the down button is held
    else if (ray->btnState & PB_DOWN)
    {
        // Move backwards
        deltaY += 1;
    }

    // If the left button is held
    if (ray->btnState & PB_LEFT)
    {
        // Move left
        deltaX -= 1;
    }
    // Else if the right button is held
    else if (ray->btnState & PB_RIGHT)
    {
        // Move backwards
        deltaX += 1;
    }

    // If there is movement
    if (deltaX || deltaY)
    {
        // Normalize deltaX and deltaY before scaling with elapsedUs
        fastNormVec(&deltaX, &deltaY);
        ray->p.dirX = deltaX;
        ray->p.dirY = deltaY;

        // Should move 1/6 units every 40000uS
        deltaX = (int32_t)(deltaX * elapsedUs) / (int32_t)(40000 * 6);
        deltaY = (int32_t)(deltaY * elapsedUs) / (int32_t)(40000 * 6);

        // Save the old cell to check for crossing cell boundaries
        int16_t oldCellX = FROM_FX(ray->p.posX);
        int16_t oldCellY = FROM_FX(ray->p.posY);

        // A little less than half the width of the player, for boundary checks
        // TODO use actual player hitbox
        // q24_8 pHalfWidth = TO_FX_FRAC(7, 16);

        // TODO use actual player dimensions
        rectangle_t movedBoundingBox = {
            .pos.x  = ray->p.posX - TO_FX_FRAC(7, 16) + deltaX,
            .pos.y  = ray->p.posY - TO_FX_FRAC(7, 16) + deltaY,
            .width  = TO_FX_FRAC(14, 16),
            .height = TO_FX_FRAC(14, 16),
        };

        // If the player's new location doesn't fit
        if (!rayBoundingBoxFitsInMap(ray, movedBoundingBox))
        {
            // Stop movement
            deltaX = 0;
            deltaY = 0;

            // TODO allow axis aligned movement when the input is diagonal on a wall?
        }

        // Check for collisions with all enemies in the new location
        node_t* eNode = ray->enemies.first;
        while (eNode)
        {
            rayEnemy_t* e = eNode->val;

            // If the player collides with an immovable enemy (i.e. a box on a wall) this will stop movement
            rayEnemyCheckCollision(ray, e, movedBoundingBox, &deltaX, &deltaY);
            eNode = eNode->next;
        }

        // Update location with allowed movement
        ray->p.posX += deltaX;
        ray->p.posY += deltaY;

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

    // Run the sword timer
    if (ray->p.swordTimerUs > 0)
    {
        ray->p.swordTimerUs -= elapsedUs;
        // SWORD_SWING_ANGLE degrees in SWORD_SWING_TIME us
        ray->p.swordAngle += (SWORD_SWING_ANGLE * elapsedUs) / SWORD_SWING_TIME;
        if (ray->p.swordAngle >= 360)
        {
            ray->p.swordAngle -= 360;
        }
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
    linearTouch_t touches[2] = {0};
    getTouchLinear(touches, ARRAY_SIZE(touches));
    const linearTouch_t* rTouch = &touches[1];

    struct touchState* ts = &ray->p.ts;

    if (rTouch->touched)
    {
        // Save initial position if not set
        if (ts->initialTouchPos < 0)
        {
            ts->initialTouchPos = rTouch->position;
        }

        // Save last touch
        ts->lastTouchPos = rTouch->position;

        // Calculate the distance between inital and curren touches
        int32_t touchDelta = rTouch->position - ts->initialTouchPos;

        // Check if the touch has dragged far enough to start drawing a bow or setting a bomb
        const int32_t touchLimit = 1024 / 5;
        if (touchDelta > touchLimit)
        {
            if (!ts->drawingBow)
            {
                ts->drawingBow = true;
            }
        }
        else if (touchDelta < -touchLimit)
        {
            if (!ts->settingBomb)
            {
                printf("Setting bomb\n");
                ts->settingBomb = true;
            }
        }
    }
    else if (ts->initialTouchPos >= 0)
    {
        // Touch has been released
        int32_t touchDelta = ts->lastTouchPos - ts->initialTouchPos;
        touchDelta         = ABS(touchDelta);

        // fire!
        if (ts->drawingBow)
        {
            ts->drawingBow = false;

            // TODO scale by touch delta
            q24_8 velX = ray->p.dirX;
            q24_8 velY = ray->p.dirY;

            velX = (velX * touchDelta) / 1024;
            velY = (velY * touchDelta) / 1024;
            rayCreateBullet(ray, OBJ_BULLET_NORMAL, ray->p.posX, ray->p.posY, velX, velY, -ray->p.dirX, -ray->p.dirY,
                            -1, true);
        }
        else if (ts->settingBomb)
        {
            ts->settingBomb = false;

            // 64 bit to prevent saturation
            int32_t fuseUs = (touchDelta * (int64_t)4000000) / 1024;

            // Spawn the bomb slightly in front of the player
            rayCreateBullet(ray, OBJ_BULLET_MISSILE, ray->p.posX + (ray->p.dirX / 2), ray->p.posY + (ray->p.dirY / 2),
                            0, 0, 0, 0, fuseUs, true);
        }

        // Reset variables
        ts->initialTouchPos = -1;
        ts->lastTouchPos    = -1;
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
            globalMidiPlayerPlaySong(&ray->sfx_health, MIDI_SFX);
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
            globalMidiPlayerPlaySong(&ray->sfx_health, MIDI_SFX);
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
        globalMidiPlayerPlaySong(&ray->sfx_item_get, MIDI_SFX);
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
            // globalMidiPlayerPlaySong(&ray->sfx_lava_dmg, MIDI_SFX);
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

        if (false == ray->playerInHealth)
        {
            ray->playerInHealth = true;
        }
    }
    else if (true == ray->playerInLava)
    {
        ray->playerInLava = false;
        // Stop looping SFX
        // ray->sfx_lava_dmg.shouldLoop = true;
    }
    else if (true == ray->playerInHealth)
    {
        ray->playerInHealth = false;
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
        globalMidiPlayerPlaySong(&ray->sfx_p_damage, MIDI_SFX);
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

/**
 * @brief TODO doc
 *
 * @param ray
 */
line_t rayGetSwordLineSegment(ray_t* ray)
{
    line_t sword = {
        .p1.x = ray->p.posX,
        .p1.y = ray->p.posY,
        .p2.x = ray->p.posX,
        .p2.y = ray->p.posY,
    };
    if (ray->p.swordTimerUs > 0)
    {
        sword.p2.x += getSin1024(ray->p.swordAngle) / 4;
        sword.p2.y += -getCos1024(ray->p.swordAngle) / 4;
    }
    return sword;
}

/**
 * @brief TODO doc
 *
 * @param ray
 * @return int32_t
 */
int32_t rayGetEightWayAngle(int16_t x, int16_t y)
{
    int32_t angle = 0;
    if (y < 0)
    {
        if (x < 0)
        {
            // Up Left
            angle = 45 * 7;
        }
        else if (x > 0)
        {
            // Up Right
            angle = 45 * 1;
        }
        else
        {
            // Up
            angle = 45 * 0;
        }
    }
    else if (y > 0)
    {
        if (x < 0)
        {
            // Down Left
            angle = 45 * 5;
        }
        else if (x > 0)
        {
            // Down Right
            angle = 45 * 3;
        }
        else
        {
            // Down
            angle = 45 * 4;
        }
    }
    else
    {
        if (x < 0)
        {
            // Left
            angle = 45 * 6;
        }
        else if (x > 0)
        {
            // Right
            angle = 45 * 2;
        }
    }
    return angle;
}
