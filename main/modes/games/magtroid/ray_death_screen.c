//==============================================================================
// Includes
//==============================================================================

#include "ray_death_screen.h"

//==============================================================================
// Constant data
//==============================================================================

static const char* gameOverTitle = "Game Over";

// TODO update death texts
static const char* const deathTexts[] = {
    "Your quest ends here",
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
    globalMidiPlayerStop(true);
    globalMidiPlayerPlaySong(&ray->sfx_game_over, MIDI_SFX);

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

    // Colors and margins
#define BG_COLOR          c344
#define TXT_BG_COLOR      c011
#define TXT_COLOR         c555
#define BORDER_MARGIN     16
#define DEATH_TEXT_MARGIN (BORDER_MARGIN + 12)
    const paletteColor_t borders[] = {c210, c430, c430, c430, c542, c210};

    // Fill light background
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, BG_COLOR);

    // Draw concentric rectangle borders
    int bIdx;
    for (bIdx = BORDER_MARGIN; bIdx < BORDER_MARGIN + ARRAY_SIZE(borders); bIdx++)
    {
        drawRect(bIdx, bIdx, TFT_WIDTH - bIdx, TFT_HEIGHT - bIdx, borders[bIdx - BORDER_MARGIN]);
    }

    // Fill dark background
    fillDisplayArea(bIdx, bIdx, TFT_WIDTH - bIdx, TFT_HEIGHT - bIdx, TXT_BG_COLOR);

    // Pick a font
    font_t* font = ray->renderer->titleFont;

    // Draw a title
    int16_t yOff   = DEATH_TEXT_MARGIN;
    int16_t tWidth = textWidth(font, gameOverTitle);
    int16_t goX    = (TFT_WIDTH - tWidth) / 2;
    drawText(font, TXT_COLOR, gameOverTitle, goX, yOff);
    yOff += font->height;

    // Draw Underline
    yOff -= 10;
    fillDisplayArea(goX, yOff, //
                    goX + tWidth, yOff + 1, TXT_COLOR);
    yOff += 1;

    // Measure the height of the wrapped text
    int16_t messageMaxHeight = (TFT_HEIGHT - DEATH_TEXT_MARGIN);
    uint16_t tHeight = textWordWrapHeight(font, ray->deathText, TFT_WIDTH - (DEATH_TEXT_MARGIN * 2), messageMaxHeight);

    // Set the offsets
    int16_t xOff = DEATH_TEXT_MARGIN;
    yOff += (((messageMaxHeight - yOff) - tHeight) / 2);

    // Draw the message
    drawTextWordWrap(font, TXT_COLOR, ray->deathText, &xOff, &yOff, TFT_WIDTH - xOff, TFT_HEIGHT);
}
