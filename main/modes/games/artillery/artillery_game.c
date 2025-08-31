//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_game.h"
#include "artillery_phys_camera.h"
#include "artillery_p2p.h"

//==============================================================================
// Defines
//==============================================================================

#define BARREL_INTERVAL      (M_PI / 180.0f)
#define POWER_INTERVAL       1.0f
#define TOUCH_DEG_PER_BARREL 8

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Switch to a new game state (menu, adjusting shot, etc.)
 *
 * @param ad All the artillery mode data
 * @param newState The state to switch to
 */
void artillerySwitchToGameState(artilleryData_t* ad, artilleryGameState_t newState)
{
    // Set the new state
    ad->gState = newState;

    // Clear all camera targets
    clear(&ad->phys->cameraTargets);

    // Additional state-specific setup
    switch (ad->gState)
    {
        case AGS_MENU:
        {
            setDriveInMenu(ad->moveTimerUs);
            break;
        }
        case AGS_CPU_ADJUST:
        {
            ad->players[ad->plIdx]->targetBarrelAngle = -1;
            ad->cpuWaitTimer                          = 2000000;
            break;
        }
        default:
        case AGS_WAIT:
        case AGS_MOVE:
        case AGS_ADJUST:
        case AGS_FIRE:
        case AGS_CPU_MOVE:
        case AGS_LOOK:
        {
            break;
        }
    }

    // Focus on player if not looking around
    if (AGS_LOOK != ad->gState)
    {
        if (ad->players[ad->plIdx])
        {
            push(&ad->phys->cameraTargets, ad->players[ad->plIdx]);
        }
    }
}

/**
 * @brief Process button events for the artillery game
 *
 * @param ad All the artillery mode data
 * @param evt The button event
 * @return true if the barrel angle changed (need to TX a packet), false otherwise
 */
bool artilleryGameInput(artilleryData_t* ad, buttonEvt_t evt)
{
    switch (ad->gState)
    {
        default:
        case AGS_WAIT:
        {
            // Do nothing!
            break;
        }
        case AGS_MENU:
        {
            // Menu navigation
            menu_t* oldMenu = ad->gameMenu;
            ad->gameMenu    = menuButton(ad->gameMenu, evt);

            // If the ammo menu was entered, scroll to current ammo
            if (oldMenu != ad->gameMenu && load_ammo == ad->gameMenu->title)
            {
                menuNavigateToItem(ad->gameMenu, ad->players[ad->plIdx]->ammoLabel);
            }
            break;
        }
        case AGS_LOOK:
        {
            // Check if the menu should be exited
            if (evt.down)
            {
                switch (evt.button)
                {
                    case PB_A:
                    case PB_B:
                    {
                        // Return to the menu
                        artillerySwitchToGameState(ad, AGS_MENU);
                        return false;
                    }
                    default:
                    {
                        break;
                    }
                }
            }

            // Set button to pan camera
            physSetCameraButton(ad->phys, evt.state);
            break;
        }
        case AGS_MOVE:
        {
            if (evt.down)
            {
                switch (evt.button)
                {
                    case PB_LEFT:
                    case PB_RIGHT:
                    {
                        // Left and right buttons set acceleration
                        ad->players[ad->plIdx]->moving = evt.button;
                        break;
                    }
                    case PB_A:
                    case PB_B:
                    {
                        artillerySwitchToGameState(ad, AGS_MENU);
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            else
            {
                // Releasing a button clears acceleration
                ad->players[ad->plIdx]->moving = 0;
            }
            break;
        }
        case AGS_ADJUST:
        {
            switch (evt.button)
            {
                case PB_LEFT:
                case PB_RIGHT:
                {
                    if (evt.down)
                    {
                        ad->adjButtonHeld       = evt.button;
                        ad->adjButtonStartTimer = 0;
                        float bDiff             = (PB_LEFT == evt.button) ? -(BARREL_INTERVAL) : (BARREL_INTERVAL);
                        setBarrelAngle(ad->players[ad->plIdx], ad->players[ad->plIdx]->barrelAngle + bDiff);
                        return true;
                    }
                    else
                    {
                        ad->adjButtonHeld       = 0;
                        ad->adjButtonStartTimer = 0;
                    }
                    break;
                }
                case PB_UP:
                case PB_DOWN:
                {
                    if (evt.down)
                    {
                        ad->adjButtonHeld       = evt.button;
                        ad->adjButtonStartTimer = 0;
                        float pDiff             = (PB_UP == evt.button) ? POWER_INTERVAL : -POWER_INTERVAL;
                        setShotPower(ad->players[ad->plIdx], ad->players[ad->plIdx]->shotPower + pDiff);
                    }
                    else
                    {
                        ad->adjButtonHeld       = 0;
                        ad->adjButtonStartTimer = 0;
                    }
                    break;
                }
                case PB_A:
                case PB_B:
                {
                    if (evt.down)
                    {
                        artillerySwitchToGameState(ad, AGS_MENU);
                    }
                    break;
                }
                default:
                {
                    break;
                }
            }
            break;
        }
        case AGS_CPU_MOVE:
        case AGS_CPU_ADJUST:
        {
            // No input while it's the CPU turn
            break;
        }
    }
    return false;
}

/**
 * @brief Run the main loop for the artillery game.
 * This includes running physics and drawing to the TFT.
 *
 * @param ad All the artillery mode data
 * @param elapsedUs The time elapsed since this was last called.
 * @param barrelChanged
 */
void artilleryGameLoop(artilleryData_t* ad, uint32_t elapsedUs, bool barrelChanged)
{
    // Step the physics
    bool physChange = physStep(ad->phys, elapsedUs);

    // Draw the scene
    drawPhysOutline(ad->phys, ad->moveTimerUs);

    // Get the system font to draw text
    font_t* f = getSysFont();

    // Draw depending on the game state
    switch (ad->gState)
    {
        case AGS_MENU:
        {
            // Menu renderer
            drawMenuSimple(ad->gameMenu, ad->smRenderer);
            break;
        }
        case AGS_ADJUST:
        {
            // Check for touchpad input
            int32_t phi       = 0;
            int32_t r         = 0;
            int32_t intensity = 0;
            if (getTouchJoystick(&phi, &r, &intensity))
            {
                // If the touchpad was previously untouched, set the last angle as current
                if (INT32_MIN == ad->tpLastPhi)
                {
                    ad->tpLastPhi = phi;
                }

                // Find the difference between the last angle and this one
                int32_t diff = ad->tpLastPhi - phi;
                if (diff > 180)
                {
                    diff -= 360;
                }
                else if (diff < -180)
                {
                    diff += 360;
                }

                // Keep track of the cumulative difference
                ad->tpCumulativeDiff += diff;

                // Adjust the barrel when enough touchpad degrees have accumulated
                while (ad->tpCumulativeDiff >= TOUCH_DEG_PER_BARREL)
                {
                    ad->tpCumulativeDiff -= TOUCH_DEG_PER_BARREL;
                    setBarrelAngle(ad->players[ad->plIdx], ad->players[ad->plIdx]->barrelAngle + BARREL_INTERVAL);
                    barrelChanged = true;
                }
                while (ad->tpCumulativeDiff <= -TOUCH_DEG_PER_BARREL)
                {
                    ad->tpCumulativeDiff += TOUCH_DEG_PER_BARREL;
                    setBarrelAngle(ad->players[ad->plIdx], ad->players[ad->plIdx]->barrelAngle - BARREL_INTERVAL);
                    barrelChanged = true;
                }

                // Save the current touchpad angle
                ad->tpLastPhi = phi;
            }
            else
            {
                ad->tpLastPhi        = INT32_MIN;
                ad->tpCumulativeDiff = 0;
            }

            // If a button is held
            if (ad->adjButtonHeld)
            {
                // Ignore it for the first quarter second
                if (ad->adjButtonStartTimer < 250000)
                {
                    ad->adjButtonStartTimer += elapsedUs;
                }
                else
                {
                    // Left/Right moves faster than Up/Down
                    int32_t timerInterval = (ad->adjButtonHeld & (PB_LEFT | PB_RIGHT)) ? 10000 : 75000;

                    // Run a periodic timer to adjust inputs
                    RUN_TIMER_EVERY(ad->adjButtonHeldTimer, timerInterval, elapsedUs, {
                        switch (ad->adjButtonHeld)
                        {
                            case PB_LEFT:
                            case PB_RIGHT:
                            {
                                float bDiff = (PB_LEFT == ad->adjButtonHeld) ? -(BARREL_INTERVAL) : (BARREL_INTERVAL);
                                setBarrelAngle(ad->players[ad->plIdx], ad->players[ad->plIdx]->barrelAngle + bDiff);
                                barrelChanged = true;
                                break;
                            }
                            case PB_UP:
                            case PB_DOWN:
                            {
                                float pDiff = (PB_UP == ad->adjButtonHeld) ? POWER_INTERVAL : -POWER_INTERVAL;
                                setShotPower(ad->players[ad->plIdx], ad->players[ad->plIdx]->shotPower + pDiff);
                                barrelChanged = true;
                                break;
                            }
                            default:
                            {
                                ad->adjButtonHeld = 0;
                                break;
                            }
                        }
                    });
                }
            }

            // Draw the shot parameters
            char fireParams[64];
            snprintf(fireParams, sizeof(fireParams) - 1, "Angle %d, Power %d",
                     (int)((180 * ad->players[ad->plIdx]->barrelAngle) / M_PI), (int)ad->players[ad->plIdx]->shotPower);
            drawText(f, c555, fireParams, 40, TFT_HEIGHT - f->height - 2);
            break;
        }
        case AGS_FIRE:
        {
            // If the shot hasn't fired yet, fire away!
            if (false == ad->phys->shotFired)
            {
                ad->phys->shotFired = true;
                fireShot(ad->phys, ad->players[ad->plIdx]);
                artilleryTxShot(ad, ad->players[ad->plIdx]);
            }
            // Run a timer to wait between switching players, otherwise it's too rushed
            else if (ad->phys->playerSwapTimerUs)
            {
                ad->phys->playerSwapTimerUs -= elapsedUs;
                if (ad->phys->playerSwapTimerUs <= 0)
                {
                    artilleryPassTurn(ad);
                    artilleryTxPassTurn(ad);
                }
            }
            break;
        }
        case AGS_MOVE:
        case AGS_CPU_MOVE:
        {
            // Pick a random direction for the CPU to move
            if ((AGS_CPU_MOVE == ad->gState) && (0 == ad->players[ad->plIdx]->moving))
            {
                ad->players[ad->plIdx]->moving = (esp_random() & 0x01) ? PB_LEFT : PB_RIGHT;
            }

            // If there is time left to move and the player is moving
            if (ad->moveTimerUs && ad->players[ad->plIdx]->moving)
            {
                // Decrement the timer
                ad->moveTimerUs -= elapsedUs;
                // If the timer expired
                if (ad->moveTimerUs <= 0)
                {
                    // Clear timer and button inputs and return to the menu
                    ad->moveTimerUs                = 0;
                    ad->players[ad->plIdx]->moving = 0;
                    // Humans go back to the menu, CPUs adjust shot
                    artillerySwitchToGameState(ad, (ad->gState == AGS_MOVE) ? AGS_MENU : AGS_CPU_ADJUST);
                }
            }
            // TODO TX gas gauge too!
            break;
        }
        case AGS_LOOK:
        case AGS_WAIT:
        {
            break;
        }
        case AGS_CPU_ADJUST:
        {
            physCirc_t* cpu = ad->players[ad->plIdx];

            // Keep track of if the barrel is positioned correctly
            bool readyToFire = false;
            if (-1 != cpu->targetBarrelAngle)
            {
                // Find the clockwise and counterclockwise distances to move the barrel to the target
                float deltaCw = cpu->barrelAngle - cpu->targetBarrelAngle;
                if (deltaCw < 0)
                {
                    deltaCw += (2 * M_PI);
                }
                float deltaCCw = cpu->targetBarrelAngle - cpu->barrelAngle;
                if (deltaCCw < 0)
                {
                    deltaCCw += (2 * M_PI);
                }

                // If the barrel isn't at the target, move barrel towards target
                if (fabsf(deltaCw) > BARREL_INTERVAL)
                {
                    if (deltaCw < deltaCCw)
                    {
                        setBarrelAngle(cpu, cpu->barrelAngle - BARREL_INTERVAL);
                    }
                    else
                    {
                        setBarrelAngle(cpu, cpu->barrelAngle + BARREL_INTERVAL);
                    }
                }
                else // Barrel is at the target
                {
                    readyToFire = true;
                }
            }

            // Wait some time after moving or to adjust the barrel
            if (ad->cpuWaitTimer > 0)
            {
                ad->cpuWaitTimer -= elapsedUs;

                if (ad->cpuWaitTimer <= 0)
                {
                    // If the target hasn't acquired yet
                    if (cpu->targetBarrelAngle < 0)
                    {
                        // Calculate the shot
                        adjustCpuShot(ad->phys, cpu, ad->players[(ad->plIdx + 1) % NUM_PLAYERS]);

                        // Round power and angle, to be fair
                        cpu->shotPower
                            = POWER_INTERVAL * (int)((cpu->shotPower / POWER_INTERVAL) + (POWER_INTERVAL / 2.0f));
                        cpu->targetBarrelAngle
                            = BARREL_INTERVAL
                              * (int)((cpu->targetBarrelAngle / BARREL_INTERVAL) + (BARREL_INTERVAL / 2.0f));

                        // Give time to move the barrel
                        ad->cpuWaitTimer = 2000000;
                    }
                    else if (readyToFire)
                    {
                        // Fire!
                        artillerySwitchToGameState(ad, AGS_FIRE);
                    }
                    else
                    {
                        // Timer elapsed, but barrel isn't ready yet, extend by a little
                        ad->cpuWaitTimer = 1;
                    }
                }
            }
            break;
        }
    }

    // If this is a wireless game and there is some change and it's our turn
    if (AG_WIRELESS == ad->gameType && (barrelChanged || physChange) && artilleryIsMyTurn(ad))
    {
        // Transmit the change to the other Swadge
        artilleryTxPlayers(ad);
    }

    // TODO remove from production
    DRAW_FPS_COUNTER((*f));
}

/**
 * @brief TODO doc
 *
 */
void artilleryPassTurn(artilleryData_t* ad)
{
    ad->phys->playerSwapTimerUs = 0;
    ad->phys->shotFired         = false;

    // Switch to the next player
    ad->plIdx = (ad->plIdx + 1) % NUM_PLAYERS;

    // Reset move timer
    ad->moveTimerUs = TANK_MOVE_TIME_US;

    // Switch to the right state depending on game type
    switch (ad->gameType)
    {
        default:
        case AG_PASS_AND_PLAY:
        {
            // Return to the menu
            artillerySwitchToGameState(ad, AGS_MENU);
            break;
        }
        case AG_CPU_PRACTICE:
        {
            artillerySwitchToGameState(ad, artilleryIsMyTurn(ad) ? AGS_MENU : AGS_CPU_MOVE);
            break;
        }
        case AG_WIRELESS:
        {
            artillerySwitchToGameState(ad, artilleryIsMyTurn(ad) ? AGS_MENU : AGS_WAIT);
            break;
        }
    }

    // Reset menu to top item
    ad->gameMenu = menuNavigateToTopItem(ad->gameMenu);
}

/**
 * @brief TODO doc
 *
 * @param ad
 * @return true if it is this player's turn, false otherwise
 */
bool artilleryIsMyTurn(artilleryData_t* ad)
{
    switch (ad->gameType)
    {
        default:
        case AG_PASS_AND_PLAY:
        {
            return true;
        }
        case AG_CPU_PRACTICE:
        {
            return (0 == ad->plIdx);
        }
        case AG_WIRELESS:
        {
            return (GOING_FIRST == p2pGetPlayOrder(&ad->p2p)) != (0 == ad->plIdx);
        }
    }
}
