//==============================================================================
// Includes
//==============================================================================

#include "ray_death_screen.h"

//==============================================================================
// Constant data
//==============================================================================

static const char* const deathTexts[] = {
    "GG no re",
    "Skill Issue",
    "Press F",
    "YOUR HEAD ASPLODE",
    "You may have died, but you are not safe from taxes",
    "Pro Tip: Shoot the enemies until they die",
    "Your suit didn't disintegrate revealing a bikini-clad bird, you just died",
    "Should have ducked",
    "I guess the galaxy will never be at peace",
    "Turns out main characters *can* die, who knew?",
    "Runtime error 0x00001337 SEGMENTATION FAULT nah just kidding you died",
    "Have you tried turning it off and then on again?",
    "Try again, but do better",
    "Remember when you used to give the controller to a friend to beat the hard levels? Yeah, good times...",
    "Game Over. Insert two quarters to continue.",
    "One life remaining... Just kidding!",
    "Would you like to purchase better armor for $5? Too bad, we don't pay to win!",
    "You seem to be experiencing cognitive and/or coordination difficulties. Emergency medical services have been "
    "alerted.",
};

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
    ray->btnLockoutUs = RAY_BUTTON_LOCKOUT_US;
    raySwitchToScreen(RAY_DEATH_SCREEN);
    // Stop BGM when dead
    bzrStop(true);
    bzrPlaySfx(&ray->sfx_game_over, BZR_RIGHT);

    // Pick random text to display
    ray->deathText = deathTexts[esp_random() % ARRAY_SIZE(deathTexts)];
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

#define DEATH_TEXT_MARGIN 25

    // Measure the height of the wrapped text
    uint16_t tHeight = textWordWrapHeight(&ray->logbook, ray->deathText, TFT_WIDTH - (DEATH_TEXT_MARGIN * 2),
                                          TFT_HEIGHT - (DEATH_TEXT_MARGIN * 2));

    // Set the offsets
    int16_t xOff = DEATH_TEXT_MARGIN;
    int16_t yOff = (TFT_HEIGHT - tHeight) / 2;

    drawTextWordWrap(&ray->logbook, c542, ray->deathText, &xOff, &yOff, TFT_WIDTH - xOff, TFT_HEIGHT - yOff);

    // Blink an arrow to show there's more dialog
    if (ray->blink && 0 == ray->btnLockoutUs)
    {
#define TRIANGLE_OFFSET 20
        drawTriangleOutlined(TFT_WIDTH - TRIANGLE_OFFSET - 16, TFT_HEIGHT - TRIANGLE_OFFSET - 4,
                             TFT_WIDTH - TRIANGLE_OFFSET - 4, TFT_HEIGHT - TRIANGLE_OFFSET - 10,
                             TFT_WIDTH - TRIANGLE_OFFSET - 16, TFT_HEIGHT - TRIANGLE_OFFSET - 16, c100, c542);
    }
}
