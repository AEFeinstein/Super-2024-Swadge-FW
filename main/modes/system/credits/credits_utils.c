//==============================================================================
// Includes
//==============================================================================

#include <string.h>
#include "hdw-led.h"
#include "soundFuncs.h"
#include "hdw-tft.h"
#include "fs_font.h"
#include "midiFileParser.h"
#include "macros.h"
#include "credits_utils.h"

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize the credits
 *
 * @param credits The credits to initialize
 * @param font A font to use for the credits
 * @param entries An array of strings and colors, one per line of the credits
 * @param numEntries The number of names (and colors) for the credits
 */
void initCredits(credits_t* credits, font_t* font, const creditsEntry_t* entries, int32_t numEntries)
{
    // Load some fonts
    credits->font = font;

    // Set initial variables
    credits->yOffset    = TFT_HEIGHT;
    credits->tElapsedUs = 0;
    credits->scrollMod  = 1;
    credits->entries    = entries;
    credits->numEntries = numEntries;

    // Init MIDI player is initialized
    initGlobalMidiPlayer();
    midiPlayer_t* player = globalMidiPlayerGet(MIDI_BGM);
    midiGmOn(player);

    // Load the MIDI file
    loadMidiFile("hd_credits.mid", &credits->song, true);
    midiSetFile(player, &credits->song);

    // Play the song
    player->loop = true;
    midiPause(player, false);

    // Turn off LEDs
    led_t leds[CONFIG_NUM_LEDS] = {0};
    setLeds(leds, CONFIG_NUM_LEDS);
}

/**
 * @brief Deinitialize the credits
 *
 * @param credits The credits to deinitialize
 */
void deinitCredits(credits_t* credits)
{
    unloadMidiFile(&credits->song);
    deinitGlobalMidiPlayer();
}

/**
 * @brief Draw the credits to the display
 *
 * @param credits The credits to draw
 * @param elapsedUs The time elapsed since this was last called
 */
void drawCredits(credits_t* credits, uint32_t elapsedUs)
{
    credits->tElapsedUs += elapsedUs;

    // If enough time has passed, translate and redraw text
    uint32_t updateTime = 100000 / ABS(credits->scrollMod);
    if (credits->tElapsedUs > updateTime)
    {
        credits->tElapsedUs -= updateTime;

        // This static var tracks the vertical scrolling offset
        credits->yOffset -= (credits->scrollMod > 0) ? 1 : -1;
    }

    // Clear first
    clearPxTft();

    // Draw names until the cursor is off the screen
    int16_t yPos = 0;
    int16_t idx  = 0;
    while ((yPos + credits->yOffset) < TFT_HEIGHT)
    {
        // Only draw names with negative offsets if they're a little on screen
        if ((yPos + credits->yOffset) >= -credits->font->height)
        {
            // If the names have scrolled back to the start, reset the scroll vars
            if (0 == (yPos + credits->yOffset) && 0 == idx)
            {
                credits->yOffset = 0;
                yPos             = 0;
            }

            // Center and draw the text
            int16_t tWidth = textWidth(credits->font, credits->entries[idx].name);
            drawText(credits->font, credits->entries[idx].color, credits->entries[idx].name, (TFT_WIDTH - tWidth) / 2,
                     (yPos + credits->yOffset));
        }

        // Add more space if the credits end in a newline
        size_t nameLen = strlen(credits->entries[idx].name);
        if ((nameLen > 0) && ('\n' == credits->entries[idx].name[nameLen - 1]))
        {
            yPos += credits->font->height + 8;
        }
        else
        {
            yPos += credits->font->height + 1;
        }

        // Always update the idx and cursor position, even if the text wasn't drawn
        idx = (idx + 1) % credits->numEntries;
    }
}

/**
 * @brief Credits button callback, either speed up, reverse, or exit credits
 *
 * @param evt The button event
 * @return true if the credits exit, false otherwise
 */
bool creditsButtonCb(credits_t* credits, buttonEvt_t* evt)
{
    if (evt->down)
    {
        switch (evt->button)
        {
            case PB_UP:
            {
                // Scroll faster
                credits->scrollMod = 4;
                break;
            }
            case PB_DOWN:
            {
                // Scroll faster, backwards
                credits->scrollMod = -4;
                break;
            }
            case PB_A:
            case PB_B:
            {
                // Exit
                return true;
            }
            case PB_LEFT:
            case PB_RIGHT:
            case PB_START:
            case PB_SELECT:
            {
                break;
            }
        }
    }
    else
    {
        switch (evt->button)
        {
            case PB_UP:
            case PB_DOWN:
            {
                // Resume normal scrolling
                credits->scrollMod = 1;
                break;
            }
            case PB_A:
            case PB_B:
            case PB_LEFT:
            case PB_RIGHT:
            case PB_START:
            case PB_SELECT:
            {
                // Do nothing
                break;
            }
        }
    }
    return false;
}
