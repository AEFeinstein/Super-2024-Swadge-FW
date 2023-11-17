//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include "swadge2024.h"
#include "credits_utils.h"
#include "mode_credits.h"
#include "mainMenu.h"

//==============================================================================
// Functions Prototypes
//==============================================================================

void creditsEnterMode(void);
void creditsExitMode(void);
void creditsMainLoop(int64_t elapsedUs);

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

    // Load a font
    font_t* creditsFont = (font_t*)calloc(1, sizeof(font_t));
    loadFont("logbook.font", creditsFont, false);

    // Initialize credits
    initCredits(credits, creditsFont, creditNames, creditColors, ARRAY_SIZE(creditNames));
}

/**
 * Exit the credits mode, free memory
 */
void creditsExitMode(void)
{
    // Free the font
    freeFont(credits->font);
    free(credits->font);
    // Deinitialize credits
    deinitCredits(credits);
    // Free memory for this mode
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
        if (creditsButtonCb(credits, &evt))
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }

    drawCredits(credits, elapsedUs);
}
