//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include "swadge2024.h"
#include "mode_credits.h"
#include "mainMenu.h"

//==============================================================================
// Functions Prototypes
//==============================================================================

void creditsEnterMode(void);
void creditsExitMode(void);
void creditsMainLoop(int64_t elapsedUs);
void creditsButtonCb(buttonEvt_t* evt);

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    font_t font;
    int64_t tElapsedUs;
    int8_t scrollMod;
    int16_t yOffset;
    song_t song;
} credits_t;

//==============================================================================
// Variables
//==============================================================================

credits_t* credits;

swadgeMode_t modeCredits = {
    .modeName                 = "Credits",
    .wifiMode                 = NO_WIFI,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = false,
    .fnEnterMode              = creditsEnterMode,
    .fnExitMode               = creditsExitMode,
    .fnMainLoop               = creditsMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = NULL,
    .fnEspNowSendCb           = NULL,
    .fnAdvancedUSB            = NULL,
};

// Everyone's here
static const char* const creditNames[] = {
    "Adam Feinstein\n",
    "",
    "",
};

// Must be same length as creditNames
static const paletteColor_t creditColors[] = {
    c031,
    c000,
    c000,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * Enter the credits mode, allocate and initialize memory
 */
void creditsEnterMode(void)
{
    // Allocate memory for this mode
    credits = (credits_t*)calloc(1, sizeof(credits_t));

    // Load some fonts
    loadFont("logbook.font", &credits->font, false);

    // Set initial variables
    credits->yOffset    = TFT_HEIGHT;
    credits->tElapsedUs = 0;
    credits->scrollMod  = 1;

    // Load and play song
    loadSong("credits.sng", &credits->song, false);
    bzrPlayBgm(&credits->song, BZR_STEREO);
}

/**
 * Exit the credits mode, free memory
 */
void creditsExitMode(void)
{
    freeFont(&credits->font);
    freeSong(&credits->song);
    free(credits);
}

/**
 * Main credits loop, draw some scrolling credits
 *
 * @param elapsedUs The time elapsed since the last call
 */
void creditsMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt;
    while (checkButtonQueueWrapper(&evt))
    {
        creditsButtonCb(&evt);
    }

    credits->tElapsedUs += elapsedUs;

    // If enough time has passed, translate and redraw text
    uint32_t updateTime = 100000 / ABS(credits->scrollMod);
    if (credits->tElapsedUs > updateTime)
    {
        credits->tElapsedUs -= updateTime;

        // This static var tracks the vertical scrolling offset
        credits->yOffset -= (credits->scrollMod > 0) ? 1 : -1;

        // Clear first
        clearPxTft();

        // Draw names until the cursor is off the screen
        int16_t yPos = 0;
        int16_t idx  = 0;
        while ((yPos + credits->yOffset) < TFT_HEIGHT)
        {
            // Only draw names with negative offsets if they're a little on screen
            if ((yPos + credits->yOffset) >= -credits->font.height)
            {
                // If the names have scrolled back to the start, reset the scroll vars
                if (0 == (yPos + credits->yOffset) && 0 == idx)
                {
                    credits->yOffset = 0;
                    yPos             = 0;
                }

                // Center and draw the text
                int16_t tWidth = textWidth(&credits->font, creditNames[idx]);
                drawText(&credits->font, creditColors[idx], creditNames[idx], (TFT_WIDTH - tWidth) / 2,
                         (yPos + credits->yOffset));
            }

            // Add more space if the credits end in a newline
            size_t nameLen = strlen(creditNames[idx]);
            if ((nameLen > 0) && ('\n' == creditNames[idx][nameLen - 1]))
            {
                yPos += credits->font.height + 8;
            }
            else
            {
                yPos += credits->font.height + 1;
            }

            // Always update the idx and cursor position, even if the text wasn't drawn
            idx = (idx + 1) % ARRAY_SIZE(creditNames);
        }
    }
}

/**
 * @brief Credits button callback, either speed up, reverse, or exit credits
 *
 * @param evt The button event
 */
void creditsButtonCb(buttonEvt_t* evt)
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
                switchToSwadgeMode(&mainMenuMode);
                break;
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
}
