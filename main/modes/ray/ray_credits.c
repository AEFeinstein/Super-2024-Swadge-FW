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
static creditsEntry_t rayCreditEntries[] = {
    {.name = "Percentage",           .color = c224},
    {.name = "Complete: 100%",       .color = c224},
    {.name = "",                     .color = c000},
    {.name = "",                     .color = c000},
    {.name = "----------------\n",   .color = c542},
    {.name = "Magtroid Pocket\n",    .color = c542},
    {.name = "----------------\n",   .color = c542},
    {.name = "",                     .color = c000},
    {.name = "~ Programming ~\n",    .color = c521},
    {.name = "Adam Feinstein\n",     .color = c521},
    {.name = "",                     .color = c000},
    {.name = "~ Art ~\n",            .color = c424},
    {.name = "Adam Feinstein\n",     .color = c424},
    {.name = "AllieCat Cosplay\n",   .color = c424},
    {.name = "Greg Lord (gplord)\n", .color = c424},
    {.name = "Kaitie Lawson\n",      .color = c424},
    {.name = "",                     .color = c000},
    {.name = "~ Music ~\n",          .color = c241},
    {.name = "Joe Newman\n",         .color = c241},
    {.name = "",                     .color = c000},
    {.name = "~ Story ~\n",          .color = c503},
    {.name = "Adam Feinstein\n",     .color = c503},
    {.name = "Joe Newman\n",         .color = c503},
    {.name = "",                     .color = c000},
    {.name = "~ Testing ~\n",        .color = c205},
    {.name = "Greg Lord (gplord)\n", .color = c205},
    {.name = "Joe Newman\n",         .color = c205},
    {.name = "Jonathan Moriarty\n",  .color = c205},
    {.name = "",                     .color = c000},
    {.name = "~ Team Lead ~\n",      .color = c440},
    {.name = "Adam Feinstein\n",     .color = c440},
    {.name = "",                     .color = c000},
    {.name = "",                     .color = c000},
    {.name = "See you next\n",       .color = c555},
    {.name = "adventure,\n",         .color = c555},
    {.name = "Bounty Hunter!\n",     .color = c555},
    {.name = "",                     .color = c000},
    {.name = "",                     .color = c000},
    {.name = "----------------\n",   .color = c555},
    {.name = "",                     .color = c000},
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

    // Write the completion string
    static char completionStr[20];
    sprintf(completionStr, "Complete: %" PRId32 "%%", getItemCompletePct(ray));
    // Swap the completion string into the credits array
    rayCreditEntries[1].name = completionStr;

    // Stop music
    bzrStop(true);

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
            bzrStop(true);
            deinitCredits(&ray->credits);
            // Return to the menu
            raySwitchToScreen(RAY_MENU);
            return;
        }
    }

    drawCredits(&ray->credits, elapsedUs);
}
