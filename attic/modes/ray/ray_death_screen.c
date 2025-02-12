//==============================================================================
// Includes
//==============================================================================

#include "ray_death_screen.h"

//==============================================================================
// Constant data
//==============================================================================

static const char* const deathTexts[] = {
    // Cheeky messages
    "GG no re",
    "Skill Issue",
    "Press F",
    "YOUR HEAD ASPLODE",
    "You may have died, but you are not safe from taxes",
    "Your suit didn't disintegrate revealing a bikini-clad bird, you just died",
    "Should have ducked",
    "I guess the galaxy will never be at peace",
    "Runtime error 0x00001337 SEGMENTATION FAULT nah just kidding you died",
    "Have you tried turning it off and then on again?",
    "Remember when you used to give the controller to a friend to beat the hard levels? Good times...",
    "Insert two quarters to continue",
    "One life remaining... Just kidding!",
    "Would you like to purchase better armor for $5? Too bad, no pay to win here!",

    // Actually helpful messages
    "Remember to check your map with the PAUSE button!",
    "You can lock onto enemies with the B button to strafe around their shots! DODGE!",
    "Try exploring previous worlds and solving puzzles to find health and ammo upgrades!",
    "Did you know that enemies have weaknesses to certain weapons?",
    "Keys and doors are color- and shape-coded for your convenience.",
    "The LEDs on the Swadge use radar technology to point to the nearest enemy.",
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
    soundStop(true);
    soundPlaySfx(&ray->sfx_game_over, BZR_RIGHT);

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

    // Draw a title
    const char* gameOverText = "Game Over";
    int16_t tWidth           = textWidth(&ray->logbook, gameOverText);
    int16_t goX              = (TFT_WIDTH - tWidth) / 2;
    drawText(&ray->logbook, c542, gameOverText, goX, 0);
    fillDisplayArea(goX, ray->logbook.height + 2, goX + tWidth, ray->logbook.height + 3, c542);

#define DEATH_TEXT_MARGIN 25

    int16_t messageMaxHeight = (TFT_HEIGHT - (ray->logbook.height + 3));

    // Measure the height of the wrapped text
    uint16_t tHeight
        = textWordWrapHeight(&ray->logbook, ray->deathText, TFT_WIDTH - (DEATH_TEXT_MARGIN * 2), messageMaxHeight);

    // Set the offsets
    int16_t xOff = DEATH_TEXT_MARGIN;
    int16_t yOff = ((messageMaxHeight - tHeight) / 2) + (ray->logbook.height + 3);

    // Draw the message
    drawTextWordWrap(&ray->logbook, c542, ray->deathText, &xOff, &yOff, TFT_WIDTH - xOff, TFT_HEIGHT);

    // Blink an arrow to show there's more dialog
    if (ray->blink && 0 == ray->btnLockoutUs)
    {
#define TRIANGLE_OFFSET 20
        drawTriangleOutlined(TFT_WIDTH - TRIANGLE_OFFSET - 16, TFT_HEIGHT - TRIANGLE_OFFSET - 4,
                             TFT_WIDTH - TRIANGLE_OFFSET - 4, TFT_HEIGHT - TRIANGLE_OFFSET - 10,
                             TFT_WIDTH - TRIANGLE_OFFSET - 16, TFT_HEIGHT - TRIANGLE_OFFSET - 16, c100, c542);
    }
}
