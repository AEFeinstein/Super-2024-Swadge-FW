//==============================================================================
// Includes
//==============================================================================

#include "ray_death_screen.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draw the foreground for the death screen and run the timer
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayDeathScreenRender(ray_t* ray, uint32_t elapsedUs)
{
    // Check the button queue
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        // If A was pressed
        if (evt.down)
        {
            // TODO wait a few secs before accepting buttons
            if ((PB_A == evt.button) || (PB_B == evt.button))
            {
                // Return to the menu
                ray->screen = RAY_MENU;
            }
        }
    }

    // Fill dark red background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c100);

    // Draw some text
    static const char deathText[] = "GG scrub git gud";
    int16_t tWidth                = textWidth(&ray->logbook, deathText);
    drawText(&ray->logbook, c542, deathText, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - ray->logbook.height) / 2);
}
