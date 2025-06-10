//==============================================================================
// Includes
//==============================================================================

#include "artillery_game.h"

//==============================================================================
// Function Declarations
//==============================================================================

void artilleryDrawMenu(artilleryData_t* ad);

//==============================================================================
// Const Variables
//==============================================================================

const struct
{
    const char* text;
    artilleryGameState_t nextState;
} menuEntries[] = {
    {
        .text      = "Move Around",
        .nextState = AGS_MOVE,
    },
    {
        .text      = "Load Ammo",
        .nextState = AGS_LOAD,
    },
    {
        .text      = "Adjust Shot",
        .nextState = AGS_ADJUST,
    },
    {
        .text      = "Fire!",
        .nextState = AGS_FIRE,
    },
};

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
            // TODO menu navigation
            if (evt.down)
            {
                switch (evt.button)
                {
                    case PB_DOWN:
                    {
                        ad->gMenuSel = (ad->gMenuSel + 1) % ARRAY_SIZE(menuEntries);
                        break;
                    }
                    case PB_UP:
                    {
                        ad->gMenuSel = (ad->gMenuSel - 1) % ARRAY_SIZE(menuEntries);
                        break;
                    }
                    case PB_A:
                    {
                        ad->gState = menuEntries[ad->gMenuSel].nextState;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
            break;
        }
        case AGS_LOAD:
        {
            if (evt.down)
            {
                switch (evt.button)
                {
                    case PB_A:
                    case PB_B:
                    {
                        // TODO shot selection
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
                        // TODO only accelerate on ground?
                        // TODO max X velocity to make tanks feel slow?
                        ad->players[0]->acc.x = (PB_LEFT == evt.button) ? -0.0000000001f : 0.0000000001f;
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
                ad->players[0]->acc.x = 0;
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
            artilleryDrawMenu(ad);
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
        case AGS_WAIT:
        case AGS_LOAD:
        case AGS_MOVE:
        {
            break;
        }
    }

    DRAW_FPS_COUNTER((*f));
}

/**
 * @brief TODO doc
 *
 * TODO cleanup
 *
 * @param ad
 */
void artilleryDrawMenu(artilleryData_t* ad)
{
#define MENU_MARGIN 16
#define MENU_HEIGHT 96
#define FONT_MARGIN 8

    fillDisplayArea(MENU_MARGIN,                            //
                    TFT_HEIGHT - MENU_HEIGHT - MENU_MARGIN, //
                    TFT_WIDTH - MENU_MARGIN,                //
                    TFT_HEIGHT - MENU_MARGIN,               //
                    c111);
    drawRect(MENU_MARGIN,                            //
             TFT_HEIGHT - MENU_HEIGHT - MENU_MARGIN, //
             TFT_WIDTH - MENU_MARGIN,                //
             TFT_HEIGHT - MENU_MARGIN,               //
             c005);

    font_t* f = getSysFont();

    int16_t yOff = TFT_HEIGHT - MENU_HEIGHT;

    for (uint32_t i = 0; i < ARRAY_SIZE(menuEntries); i++)
    {
        drawText(f, c555, menuEntries[i].text, 2 * MENU_MARGIN, yOff);
        if (ad->gMenuSel == i)
        {
            drawText(f, c555, ">", 2 * MENU_MARGIN - 10, yOff);
        }
        yOff += f->height + FONT_MARGIN;
    }
}
