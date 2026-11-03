//==============================================================================
// Includes
//==============================================================================

#include "ray_instrument.h"
#include "ray_script.h"

//==============================================================================
// Const data
//==============================================================================

static const paletteColor_t noteColors[] = {c500, c530, c250, c051, c055, c015, c205, c503};
static const uint32_t noteFreqs[]        = {
    262, // C4
    294, // D4
    330, // E4
    349, // F4
    392, // G4
    440, // A4
    494, // B4
    523, // C5
};

const char str_sing[]    = "Sing Tomi!";
const char str_listen[]  = "Listen Tomi!";
const char str_repeat[]  = "Repeat Tomi!";
const char str_lullaby[] = "Doria's Lullaby";

//==============================================================================
// Functions Prototypes
//==============================================================================

static void handleRayInstrument(ray_t* ray, const linearTouch_t* touch, int8_t* lastTouchZone, uint32_t noteOffset,
                                synthOscillator_t* osc);
static void rayAutoPlayInstrumentNextNote(ray_t* ray);

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
            if (!ray->instrumentTutorial)
            {
                // Any button returns to the game if the tutorial isn't happening
                raySwitchToScreen(RAY_GAME);
                globalMidiPlayerResumeAll();
            }
            return;
        }
    }

    if (!ray->instrumentTutorialAutoPlay)
    {
        // Read touchpads
        linearTouch_t touches[2] = {0};
        getTouchLinear(touches, ARRAY_SIZE(touches));
        // Handle instrument inputs
        handleRayInstrument(ray, &touches[0], &ray->is.lTouchZone, 0, &ray->lOsc);
        handleRayInstrument(ray, &touches[1], &ray->is.rTouchZone, 4, &ray->rOsc);
    }
}

/**
 * @brief Handle touches for the instrument
 *
 * @param ray The entire game state
 * @param touch The touch to handle
 * @param lastTouchZone The last zone touched, to check if the zone changed
 * @param noteOffset The note offset (the second touch strip is offset by 4)
 * @param osc The oscillator to play notes with
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

            // Check for played songs if it's not auto-playing
            if (!ray->instrumentTutorialAutoPlay)
            {
                // Queue the note for song checking
                ray->is.noteHistory <<= 4;
                ray->is.noteHistory |= noteIdx;

                // Check for songs played, trigger actions
                if (0x00744045 == (ray->is.noteHistory & 0xFFFFFFF))
                {
                    // Save text to indicate what was played
                    ray->is.lastPlayedSong = str_lullaby;

                    // Clear note history
                    ray->is.noteHistory = 0xFFFFFFFF;

                    // TODO Auto-play longer song, then do the stuff below

                    // If this is the tutorial
                    if (ray->instrumentTutorial)
                    {
                        // Switch back to the cutscene
                        raySwitchToScreen(RAY_CUTSCENE);
                    }
                    else
                    {
                        // For non-tutorial, ccheck scripts for this song
                        checkScriptSong(ray, FROM_FX(ray->p.posX), FROM_FX(ray->p.posY), LULLABY, NULL);
                        raySwitchToScreen(RAY_GAME);
                    }
                }

                // If we're in the tutorial
                if (ray->instrumentTutorial)
                {
                    // Count the notes played
                    ray->is.numNotesPlayed++;
                    // If the player played at least eight notes without a song
                    if (8 <= ray->is.numNotesPlayed)
                    {
                        // Stop playing notes
                        linearTouch_t stop = {
                            .intensity = 0,
                            .position  = 0,
                            .touched   = false,
                        };
                        handleRayInstrument(ray, &stop, &ray->is.lTouchZone, 0, &ray->lOsc);
                        handleRayInstrument(ray, &stop, &ray->is.rTouchZone, 4, &ray->rOsc);

                        // Auto play again
                        ray->instrumentTutorialAutoPlay = true;
                        ray->instrumentAutoPlayIdx      = 0;
                        ray->is.numNotesPlayed          = 0;
                    }
                }
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
    // Run a timer to play demo songs
    if (ray->instrumentTutorialAutoPlay)
    {
        RUN_TIMER_EVERY(ray->instrumentAutoPlayTimer, 100000, elapsedUs, { rayAutoPlayInstrumentNextNote(ray); });
    }

    // Pick the text to draw
    const char* text = str_sing;
    if (ray->instrumentTutorial)
    {
        if (ray->instrumentTutorialAutoPlay)
        {
            text = str_listen;
        }
        else
        {
            text = str_repeat;
        }
    }
    else if (ray->is.lastPlayedSong)
    {
        text = ray->is.lastPlayedSong;
    }

    // Draw the text
    const int16_t step = (TFT_HEIGHT / 4);
    int16_t xOff       = step;
    int16_t yOff       = step;
    font_t* f          = ray->renderer->titleFont;
    drawTextWordWrapCentered(f, c000, text, &xOff, &yOff, TFT_WIDTH - step, TFT_HEIGHT);

    // Draw blocks for the eight notes, four on each side
    xOff = 0;
    yOff = 0;
    for (int i = 0; i < 8; i++)
    {
        // Either draw an outline or fill the box
        if (ray->is.notesActive & (1 << i))
        {
            fillDisplayArea(xOff, yOff, xOff + step, yOff + step, noteColors[i]);
        }
        else
        {
            drawRect(xOff, yOff, xOff + step, yOff + step, noteColors[i]);
        }

        // Step down
        yOff += step;

        // If offscreen, shift right
        if (yOff >= TFT_HEIGHT)
        {
            yOff = 0;
            xOff = TFT_WIDTH - step;
        }
    }
}

/**
 * @brief This function is called every 100ms, and controls auto-playing the Lullaby
 *
 * @param ray The entire game state
 */
static void rayAutoPlayInstrumentNextNote(ray_t* ray)
{
    // Doria's Lullaby
    const int8_t noteSequence[][2] = {
        {0, -1},  // C4
        {-1, 3},  // C5
        {-1, 0},  // G4
        {-1, 0},  // G4
        {0, -1},  // C4
        {-1, 0},  // G4
        {-1, 1},  // A4
        {-1, -1}, // Rest
    };

    // Static variables aren't great, but I am lazy!
    static uint8_t tick = 0;

    if (0 == tick)
    {
        // Start notes
        linearTouch_t touches[2] = {
            {
                .intensity = 262144,
                .position  = noteSequence[ray->instrumentAutoPlayIdx][0] * 256,
                .touched   = noteSequence[ray->instrumentAutoPlayIdx][0] >= 0,
            },
            {
                .intensity = 262144,
                .position  = noteSequence[ray->instrumentAutoPlayIdx][1] * 256,
                .touched   = noteSequence[ray->instrumentAutoPlayIdx][1] >= 0,
            },
        };
        handleRayInstrument(ray, &touches[0], &ray->is.lTouchZone, 0, &ray->lOsc);
        handleRayInstrument(ray, &touches[1], &ray->is.rTouchZone, 4, &ray->rOsc);

        // And tick
        tick++;
    }
    else if (8 > tick)
    {
        // Keep playing
        tick++;
    }
    else if (8 == tick)
    {
        // Stop notes
        linearTouch_t touches[2] = {
            {
                .intensity = 0,
                .position  = 0,
                .touched   = false,
            },
            {
                .intensity = 0,
                .position  = 0,
                .touched   = false,
            },
        };
        handleRayInstrument(ray, &touches[0], &ray->is.lTouchZone, 0, &ray->lOsc);
        handleRayInstrument(ray, &touches[1], &ray->is.rTouchZone, 4, &ray->rOsc);

        // And tick
        tick++;
    }
    else if (10 > tick)
    {
        // Keep resting
        tick++;
    }
    else if (10 == tick)
    {
        // Increment to next note
        ray->instrumentAutoPlayIdx++;

        // If all notes were played
        if (ARRAY_SIZE(noteSequence) == ray->instrumentAutoPlayIdx)
        {
            // Let the player try to repeat the song
            ray->instrumentTutorialAutoPlay = false;
            ray->instrumentAutoPlayIdx      = 0;
        }

        // Reset tick to play the next note
        tick = 0;
    }
}
