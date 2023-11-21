//==============================================================================
// Includes
//==============================================================================

#include "ray_credits.h"
#include "credits_utils.h"
#include "ray_pause.h"

//==============================================================================
// Variables
//==============================================================================

/// @brief Credits text
static const char* rayCreditNames[] = {
    "Percentage",
    "Complete: 100%",
    "",
    "",
    "----------------\n",
    "Magtroid Pocket\n",
    "----------------\n",
    "",
    "~ Programming ~\n",
    "Adam Feinstein\n",
    "",
    "~ Art ~\n",
    "Adam Feinstein\n",
    "AllieCat Cosplay\n",
    "Greg Lord (gplord)\n",
    "Kaitie Lawson\n",
    "",
    "~ Music ~\n",
    "Joe Newman\n",
    "",
    "~ Story ~\n",
    "Adam Feinstein\n",
    "Joe Newman\n",
    "",
    "~ Testing ~\n",
    "Greg Lord (gplord)\n",
    "Joe Newman\n",
    "Jonathan Moriarty\n",
    "",
    "~ Team Lead ~\n",
    "Adam Feinstein\n",
    "",
    "",
    "See you next\n",
    "adventure,\n",
    "Bounty Hunter!\n",
    "",
    "",
    "----------------\n",
    "",
};

/// @brief Credits colors.  Must be same length as rayCreditNames
static const paletteColor_t rayCreditColors[] = {
    c224, c224, c000, c000, c542, c542, c542, c000, c521, c521, c000, c424, c424, c424,
    c424, c424, c000, c241, c241, c000, c503, c503, c503, c000, c205, c205, c205, c205,
    c000, c440, c440, c000, c000, c555, c555, c555, c000, c000, c555, c000,
};

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
    rayCreditNames[1] = completionStr;

    // Stop music
    bzrStop(true);

    // Init credits, which starts music
    initCredits(&ray->credits, &ray->logbook, rayCreditNames, rayCreditColors, ARRAY_SIZE(rayCreditNames));
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
