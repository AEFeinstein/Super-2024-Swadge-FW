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
    {.name = "Adam Feinstein\n", .color = c031},
    {.name = "Bryce Browner\n", .color = c315},
    {.name = "Carrie Wood\n", .color = c203},
    {.name = "cnlohr\n", .color = c541},
    {.name = "Dac\n", .color = c545},
    {.name = "dylwhich\n", .color = c055},
    {.name = "Emily Anthony\n", .color = c435},
    {.name = "Eriktronic101\n", .color = c135},
    {.name = "Gerald M. (GEMISIS)\n", .color = c510},
    {.name = "Grav\n", .color = c130},
    {.name = "Greg Lord (gplord)\n", .color = c025},
    {.name = "Heather \"Heathstaa\"", .color = c445},
    {.name = "Hamilton\n", .color = c445},
    {.name = "Hunter Dyar\n", .color = c433},
    {.name = "James Albracht\n", .color = c440},
    {.name = "Jeremy Stintzcum\n", .color = c032},
    {.name = "Joe Newman\n", .color = c245},
    {.name = "Jon Vega", .color = c250},
    {.name = "(JVeg199X)\n", .color = c250},
    {.name = "Kaitie Lawson\n", .color = c551},
    {.name = "Kevin \"PF3k\" Lin\n", .color = c250},
    {.name = "Livingston Rampey\n", .color = c205},
    {.name = "Logan Tucker\n", .color = c522},
    {.name = "LunaToon\n", .color = c513},
    {.name = "Nathan M. Schultz\n", .color = c311},
    {.name = "Pixel\n", .color = c143},
    {.name = "Producer Scott\n", .color = c015},
    {.name = "Swadgeman (jfrye)\n", .color = c505},
    {.name = "Thaeli\n", .color = c425},
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
    // Allocate memory for this mode
    credits = (credits_t*)heap_caps_calloc(1, sizeof(credits_t), MALLOC_CAP_8BIT);

    // Load a font
    font_t* creditsFont = (font_t*)heap_caps_calloc(1, sizeof(font_t), MALLOC_CAP_8BIT);
    loadFont("sonic.font", creditsFont, false);

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
    // DRAW_FPS_COUNTER(*credits->font);
}
