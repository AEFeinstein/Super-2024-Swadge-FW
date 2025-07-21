//==============================================================================
// Includes
//==============================================================================

#include "artillery_game.h"
#include "artillery_phys_camera.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief TODO doc
 *
 * @param ad
 * @param newState
 */
void artillerySwitchToState(artilleryData_t* ad, artilleryGameState_t newState)
{
    // Set the new state
    ad->gState = newState;

    // Clear all camera targets
    clear(&ad->phys->cameraTargets);

    // Additional state-specific setup
    switch (ad->gState)
    {
        default:
        case AGS_WAIT:
        case AGS_MENU:
        case AGS_MOVE:
        case AGS_ADJUST:
        case AGS_FIRE:
        {
            // Focus on player
            push(&ad->phys->cameraTargets, ad->players[ad->plIdx]);
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
 * @brief TODO doc
 *
 * TODO cleanup
 *
 * @param ad
 * @param evt
 */
void artilleryGameInput(artilleryData_t* ad, buttonEvt_t evt)
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
                        artillerySwitchToState(ad, AGS_MENU);
                        return;
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
                        // TODO only accelerate on ground?
                        // TODO max X velocity to make tanks feel slow?
                        // ad->players[ad->plIdx]->acc.x = (PB_LEFT == evt.button) ? -0.0000000001f : 0.0000000001f;
                        break;
                    }
                    case PB_A:
                    case PB_B:
                    {
                        artillerySwitchToState(ad, AGS_MENU);
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
                        artillerySwitchToState(ad, AGS_MENU);
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
}

/**
 * @brief TODO doc
 *
 * TODO cleanup
 *
 * @param ad
 * @param elapsedUs
 */
void artilleryGameLoop(artilleryData_t* ad, uint32_t elapsedUs)
{
    physAdjustCamera(ad->phys, elapsedUs);
    physStep(ad->phys, elapsedUs);
    drawPhysOutline(ad->phys);

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
            char fireParams[64];
            snprintf(fireParams, sizeof(fireParams) - 1, "Angle %0.3f, Power %.3f", ad->players[ad->plIdx]->barrelAngle,
                     ad->players[ad->plIdx]->shotPower * 100);
            drawText(f, c555, fireParams, 40, TFT_HEIGHT - f->height - 2);
            break;
        }
        case AGS_FIRE:
        {
            if (false == ad->phys->shotFired)
            {
                ad->phys->shotFired = true;
                fireShot(ad->phys, ad->players[ad->plIdx]);
            }
            else if (ad->phys->playerSwapTimerUs)
            {
                ad->phys->playerSwapTimerUs -= elapsedUs;
                if (ad->phys->playerSwapTimerUs <= 0)
                {
                    ad->phys->playerSwapTimerUs = 0;
                    ad->phys->shotFired         = false;

                    // Switch to the next player
                    ad->plIdx = (ad->plIdx + 1) % NUM_PLAYERS;
                    artillerySwitchToState(ad, AGS_MENU);

                    // Reset menu to top item
                    ad->gameMenu = menuNavigateToTopItem(ad->gameMenu);
                }
            }
            break;
        }
        case AGS_LOOK:
        case AGS_WAIT:
        case AGS_MOVE:
        {
            break;
        }
    }

    DRAW_FPS_COUNTER((*f));
}
