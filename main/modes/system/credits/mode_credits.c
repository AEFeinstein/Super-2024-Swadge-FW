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
static const creditsEntry_t entries[] = {
    {.name = "Adam Feinstein\n", .color = c230},
    {.name = "Andy (Illiterate)\n", .color = c520},
    {.name = "crobi\n", .color = c044},
    {.name = "Dac\n", .color = c515},
    {.name = "Emily Anthony\n", .color = c104},
    {.name = "Greg Lord\n", .color = c035},
    {.name = "Heather HeathStaa\n", .color = c335},
    {.name = "James Albracht\n", .color = c552},
    {.name = "Jarett Millard\n", .color = c341},
    {.name = "Jeremy Stintzcum\n", .color = c241},
    {.name = "Joe \"Newmajoe\"", .color = c045},
    {.name = "Newman\n", .color = c045},
    {.name = "Kaitie Muncie\n", .color = c500},
    {.name = "Livingston Rampey\n", .color = c215},
    {.name = "Logan Tucker\n", .color = c450},
    {.name = "Luna Toon\n", .color = c445},
    {.name = "Nick. Harman\n", .color = c305},
    {.name = "objet discret\n", .color = c345},
    {.name = "Swadgeman (jfrye)\n", .color = c505},
    {.name = "Thaeli\n", .color = c145},
    {.name = "", .color = c000},
    {.name = "", .color = c000},
    {.name = "Thanks for", .color = c524},
    {.name = "Swadging!\n", .color = c524},
    {.name = "", .color = c000},
    {.name = "See you next year!\n", .color = c524},
    {.name = "", .color = c000},
    {.name = "", .color = c000},
    {.name = "", .color = c000},
    {.name = "", .color = c000},
};

//==============================================================================
// Functions
//==============================================================================

/**
 * Enter the credits mode, allocate and initialize memory
 */
void creditsEnterMode(void)
{
    setFrameRateUs(1000000/50);

    // Allocate memory for this mode
    credits = (credits_t*)heap_caps_calloc(1, sizeof(credits_t), MALLOC_CAP_8BIT);

    // Load a font
    font_t* creditsFont = (font_t*)heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_8BIT);
    loadFont(OXANIUM_FONT, creditsFont, false);

    // Initialize credits
    initCredits(credits, creditsFont, entries, ARRAY_SIZE(entries));
}

/**
 * Exit the credits mode, free memory
 */
void creditsExitMode(void)
{
    // Free the font
    freeFont(credits->font);
    heap_caps_free(credits->font);
    // Deinitialize credits
    deinitCredits(credits);
    // Free memory for this mode
    heap_caps_free(credits);
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
    // DRAW_FPS_COUNTER((*credits->font));
}
