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

const char creditsName[] = "Credits";

swadgeMode_t modeCredits = {
    .modeName                 = creditsName,
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
    "AllieCat Cosplay\n",
    "angrypolarbear\n",
    "Bryce Browner\n",
    "cnlohr\n",
    "dylwhich\n",
    "Emily Anthony\n",
    "Eriktronic\n",
    "Greg Lord (gplord)\n",
    "J.Vega (@JVeg199X)\n",
    "Joe Newman\n",
    "Jonathan Moriarty\n",
    "Kaitie Lawson\n",
    "MrTroy\n",
    "VanillyNeko\n",
    "",
    "",
};

// Must be same length as creditNames
static const paletteColor_t creditColors[] = {
    c031, c533, c135, c115, c520, c035, c523, c135, c045, c250, c250, c215, c550, c555, c545, c000, c000,
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
