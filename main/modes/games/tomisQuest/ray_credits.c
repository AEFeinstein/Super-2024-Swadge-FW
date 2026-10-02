//==============================================================================
// Includes
//==============================================================================

#include "ray_credits.h"
#include "credits_utils.h"
#include "ray_pause.h"

//==============================================================================
// Variables
//==============================================================================

// clang-format off
/// @brief Credits text
/// TODO update credits
static const creditsEntry_t rayCreditEntries[] = {
    {.name = "----------------\n", .color = c445},
    {.name = "Tomi's Quest\n",     .color = c445},
    {.name = "----------------\n", .color = c445},
    {.name = "",                   .color = c000},
    {.name = "~ Story ~\n",        .color = c503},
    {.name = "Joe Newman\n",       .color = c503},
    {.name = "",                   .color = c000},
    {.name = "~ Programming ~\n",  .color = c521},
    {.name = "Adam Feinstein\n",   .color = c521},
    {.name = "",                   .color = c000},
    {.name = "~ Music ~\n",        .color = c241},
    {.name = "Joe Newman\n",       .color = c241},
    {.name = "",                   .color = c000},
    {.name = "~ Art ~\n",          .color = c424},
    {.name = "Adam Feinstein\n",   .color = c424},
    {.name = "",                   .color = c000},
    {.name = "~ Testing ~\n",      .color = c205},
    {.name = "",                   .color = c000},
    {.name = "I AM ERROR\n",       .color = c555},
    {.name = "",                   .color = c000},
    {.name = "",                   .color = c000},
    {.name = "----------------\n", .color = c555},
    {.name = "",                   .color = c000},
};
// clang-format on

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Show the credits screen and set up the button lockout
 *
 * @param ray The entire game state
 */
void rayShowCredits(ray_t* ray)
{
    ray->btnLockoutUs = RAY_BUTTON_LOCKOUT_US;

    // Make sure player data is loaded for the completion string
    rayStartGame();
    raySwitchToScreen(RAY_CREDITS);

    // Stop music
    globalMidiPlayerStop(true);

    // Init credits, which starts music
    initCredits(&ray->credits, &ray->logbook, rayCreditEntries, ARRAY_SIZE(rayCreditEntries));
}

/**
 * @brief Draw the foreground for the credits screen and run the timer
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayCreditsRender(ray_t* ray, uint32_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (creditsButtonCb(&ray->credits, &evt))
        {
            globalMidiPlayerStop(true);
            deinitCredits(&ray->credits);
            // Return to the menu
            raySwitchToScreen(RAY_MENU);
            return;
        }
    }

    drawCredits(&ray->credits, elapsedUs);
}
