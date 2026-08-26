//==============================================================================
// Includes
//==============================================================================

#include "ray_instrument.h"

//==============================================================================
// Const data
//==============================================================================

static const paletteColor_t noteColors[] = {c500, c530, c250, c051, c055, c015, c205, c503};
static const uint32_t noteFreqs[]        = {262, 294, 330, 349, 392, 440, 494, 523};

//==============================================================================
// Functions Prototypes
//==============================================================================

static void handleRayInstrument(ray_t* ray, const linearTouch_t* touch, int8_t* lastTouchZone, uint32_t noteOffset,
                                synthOscillator_t* osc);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Check for button input while showing instrument and either show the next part of the instrument or return to
 * the game loop
 *
 * @param ray The entire game state
 */
void rayInstrumentCheckButtons(ray_t* ray)
{
    // Check the button queue
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        if (evt.down)
        {
            // Any button returns to the game
            raySwitchToScreen(RAY_GAME);
            globalMidiPlayerResumeAll();
            return;
        }
    }

    // Read touchpads
    linearTouch_t touches[2] = {0};
    getTouchLinear(touches, ARRAY_SIZE(touches));
    // Handle instrument inputs
    handleRayInstrument(ray, &touches[0], &ray->is.lTouchZone, 0, &ray->lOsc);
    handleRayInstrument(ray, &touches[1], &ray->is.rTouchZone, 4, &ray->rOsc);
}

/**
 * @brief TODO doc
 *
 * @param ray
 * @param touch
 * @param lastTouchZone
 * @param noteOffset
 * @param osc
 */
static void handleRayInstrument(ray_t* ray, const linearTouch_t* touch, int8_t* lastTouchZone, uint32_t noteOffset,
                                synthOscillator_t* osc)
{
    // If there was a left touch
    if (touch->touched)
    {
        // Find the touch zone
        int32_t touchZone = touch->position / (1024 / 4);

        // If the zone changed
        if (touchZone != *lastTouchZone)
        {
            // Save the new zone
            *lastTouchZone = touchZone;

            // Get the note index
            uint8_t noteIdx = touchZone + noteOffset;

            // Play the note
            swSynthSetFreq(osc, noteFreqs[noteIdx]);
            swSynthSetVolume(osc, 127);

            // Set the bit for display
            ray->is.notesActive &= (0 == noteOffset ? 0xF0 : 0x0F);
            ray->is.notesActive |= (1 << noteIdx);

            // Queue the note for song checking
            ray->is.noteHistory <<= 4;
            ray->is.noteHistory |= noteIdx;

            // Check for songs played, trigger actions
            if (0x00210210 == (ray->is.noteHistory & 0xFFFFFF))
            {
                // Auto-play longer song, switch back to RAY_GAME

                // Save text to indicate what was played
                static const char hcb[] = "Hot Cross Buns";
                ray->is.lastPlayedSong  = hcb;

                // Clear note history
                ray->is.noteHistory = 0xFFFFFFFF;
            }
        }
    }
    else if (*lastTouchZone >= 0)
    {
        // There's a saved zone, but no touch, so clear it
        swSynthSetVolume(osc, 0);
        ray->is.notesActive &= (0 == noteOffset ? 0xF0 : 0x0F);
        *lastTouchZone = -1;
    }
}

/**
 * @brief Render the current instrument box
 *
 * @param ray The entire game state
 * @param elapsedUs The elapsed time since this function was last called
 */
void rayInstrumentRender(ray_t* ray, uint32_t elapsedUs)
{
    font_t* f = ray->renderer->titleFont;

    // Draw title to indicate the mode
    const char instrumentTitle[] = "Sing Tomi!";
    int16_t yOff                 = (TFT_HEIGHT - f->height) / 2;
    drawText(f, c500, instrumentTitle, (TFT_WIDTH - textWidth(f, instrumentTitle)) / 2, yOff);

    // Draw text if a song was played
    if (ray->is.lastPlayedSong)
    {
        yOff += f->height + 2;
        drawText(f, c500, ray->is.lastPlayedSong, (TFT_WIDTH - textWidth(f, ray->is.lastPlayedSong)) / 2, yOff);
    }

    // Draw blocks for the eight notes
    const int step = (TFT_WIDTH / 8);
    for (int i = 0; i < 8; i++)
    {
        if (ray->is.notesActive & (1 << i))
        {
            fillDisplayArea(i * step, 200, (i + 1) * step, TFT_HEIGHT, noteColors[i]);
        }
        else
        {
            drawRect(i * step, 200, (i + 1) * step, TFT_HEIGHT, noteColors[i]);
        }
    }
}
