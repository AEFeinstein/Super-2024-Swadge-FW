//==============================================================================
// Includes
//==============================================================================

#include "artillery.h"
#include "artillery_game.h"
#include "artillery_phys_camera.h"
#include "artillery_p2p.h"

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
        }
        // fall through
        default:
        case AGS_WAIT:
        case AGS_MOVE:
        case AGS_ADJUST:
        case AGS_FIRE:
        {
            // Focus on player
            if (ad->players[ad->plIdx])
            {
                push(&ad->phys->cameraTargets, ad->players[ad->plIdx]);
            }
            break;
        }
        case AGS_LOOK:
        {
            // Leave cameraTargets empty
            break;
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
            if (evt.down)
            {
                switch (evt.button)
                {
                    case PB_LEFT:
                    case PB_RIGHT:
                    {
                        float bDiff = 4 * ((PB_LEFT == evt.button) ? -(M_PI / 180.0f) : (M_PI / 180.0f));
                        setBarrelAngle(ad->players[ad->plIdx], ad->players[ad->plIdx]->barrelAngle + bDiff);
                        return true;
                        break;
                    }
                    case PB_UP:
                    case PB_DOWN:
                    {
                        float pDiff = (PB_UP == evt.button) ? 0.00001f : -0.00001f;
                        setShotPower(ad->players[ad->plIdx], ad->players[ad->plIdx]->shotPower + pDiff);
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
    bool change = barrelChanged;

    // Run camera and physics for non-wireless modes, or if it's our turn
    if (AG_WIRELESS != ad->gameType || ad->myTurn)
    {
        change |= physStep(ad->phys, elapsedUs);
    }
    // Always draw
    drawPhysOutline(ad->phys, ad->moveTimerUs);

    // If this is a wireless game and there is some change and it's our turn
    if (AG_WIRELESS == ad->gameType && change && ad->myTurn)
    {
        // Transmit the change to the other Swadge
        artilleryTxPlayers(ad);
    }

    // Get the system font to draw text
    font_t* f = getSysFont();

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
            // Draw the shot parameters
            char fireParams[64];
            snprintf(fireParams, sizeof(fireParams) - 1, "Angle %0.3f, Power %.3f", ad->players[ad->plIdx]->barrelAngle,
                     ad->players[ad->plIdx]->shotPower * 100);
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
            }
            // Run a timer to wait between switching players, otherwise it's too rushed
            else if (ad->phys->playerSwapTimerUs)
            {
                ad->phys->playerSwapTimerUs -= elapsedUs;
                if (ad->phys->playerSwapTimerUs <= 0)
                {
                    ad->phys->playerSwapTimerUs = 0;
                    ad->phys->shotFired         = false;

                    // Switch to the next player
                    ad->plIdx = (ad->plIdx + 1) % NUM_PLAYERS;

                    // Reset move timer
                    ad->moveTimerUs = TANK_MOVE_TIME_US;

                    // Return to the menu
                    artillerySwitchToGameState(ad, AGS_MENU);

                    // Reset menu to top item
                    ad->gameMenu = menuNavigateToTopItem(ad->gameMenu);

                    // TODO TX packet to the other swadge
                }
            }
            break;
        }
        case AGS_MOVE:
        {
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
                    artillerySwitchToGameState(ad, AGS_MENU);
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
    }

    // TODO remove from production
    DRAW_FPS_COUNTER((*f));
}
