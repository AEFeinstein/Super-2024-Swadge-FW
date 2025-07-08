//==============================================================================
// Includes
//==============================================================================

#include "artillery_game.h"

//==============================================================================
// Function Declarations
//==============================================================================

//==============================================================================
// Const Variables
//==============================================================================

//==============================================================================
// Functions
//==============================================================================

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
            ad->gameMenu = menuButton(ad->gameMenu, evt);
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
                        ad->gState = AGS_MENU;
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
                        ad->players[0]->moving = evt.button;
                        // TODO only accelerate on ground?
                        // TODO max X velocity to make tanks feel slow?
                        // ad->players[0]->acc.x = (PB_LEFT == evt.button) ? -0.0000000001f : 0.0000000001f;
                        break;
                    }
                    case PB_A:
                    case PB_B:
                    {
                        ad->gState = AGS_MENU;
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
                ad->players[0]->moving = 0;
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
                        setBarrelAngle(ad->players[0], ad->players[0]->barrelAngle + bDiff);
                        break;
                    }
                    case PB_UP:
                    case PB_DOWN:
                    {
                        float pDiff = (PB_UP == evt.button) ? 0.00001f : -0.00001f;
                        setShotPower(ad->players[0], ad->players[0]->shotPower + pDiff);
                        break;
                    }
                    case PB_A:
                    case PB_B:
                    {
                        ad->gState = AGS_MENU;
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
    // Uncomment for autofire
    // RUN_TIMER_EVERY(ad->autofire, 100000, elapsedUs, { fireShot(ad->phys, ad->players[0]); });

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
            snprintf(fireParams, sizeof(fireParams) - 1, "Angle %0.3f, Power %.3f", ad->players[0]->barrelAngle,
                     ad->players[0]->shotPower * 100);
            drawText(f, c555, fireParams, 40, TFT_HEIGHT - f->height - 2);
            break;
        }
        case AGS_FIRE:
        {
            fireShot(ad->phys, ad->players[0]);
            ad->gState = AGS_MENU;
            break;
        }
        case AGS_LOOK:
        {
            // Run timer to pan camera based on button state
            physAdjustCamera(ad->phys, elapsedUs);
            break;
        }
        case AGS_WAIT:
        case AGS_MOVE:
        {
            break;
        }
    }

    DRAW_FPS_COUNTER((*f));
}
