//==============================================================================
// Includes
//==============================================================================

#include "ray_death_screen.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Show the death screen and set up the button lockout
 *
 * @param ray The entire game state
 */
void rayShowDeathScreen(ray_t* ray)
{
    ray->btnLockoutUs = 2000000;
    raySwitchToScreen(RAY_DEATH_SCREEN);
    // Stop BGM when dead
    bzrStop(true);
    bzrPlaySfx(&ray->sfx_game_over, BZR_RIGHT);
}

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
        if (0 == ray->btnLockoutUs)
        {
            // If A was pressed
            if (evt.down)
            {
                if ((PB_A == evt.button) || (PB_B == evt.button))
                {
                    // Return to the menu
                    raySwitchToScreen(RAY_MENU);
                }
            }
        }
    }

    // Fill dark red background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c100);

    // Draw some text
    static const char deathText[] = "GG scrub git gud";
    int16_t tWidth                = textWidth(&ray->logbook, deathText);
    drawText(&ray->logbook, c542, deathText, (TFT_WIDTH - tWidth) / 2, (TFT_HEIGHT - ray->logbook.height) / 2);

    // Blink an arrow to show there's more dialog
    if (ray->blink && 0 == ray->btnLockoutUs)
    {
#define TRIANGLE_OFFSET 20
        drawTriangleOutlined(TFT_WIDTH - TRIANGLE_OFFSET - 16, TFT_HEIGHT - TRIANGLE_OFFSET - 4,
                             TFT_WIDTH - TRIANGLE_OFFSET - 4, TFT_HEIGHT - TRIANGLE_OFFSET - 10,
                             TFT_WIDTH - TRIANGLE_OFFSET - 16, TFT_HEIGHT - TRIANGLE_OFFSET - 16, c100, c542);
    }
}
