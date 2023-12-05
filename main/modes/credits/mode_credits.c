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

// clang-format off
// Everyone's here
static const creditsEntry_t entries[] = {
    {.name = "Adam Feinstein\n",       .color = c031},
    {.name = "AllieCat Cosplay\n",     .color = c533},
    {.name = "angrypolarbear\n",       .color = c135},
    {.name = "Bryce Browner\n",        .color = c115},
    {.name = "cnlohr\n",               .color = c520},
    {.name = "Dac\n",                  .color = c252},
    {.name = "Decnav\n",               .color = c005},
    {.name = "dylwhich\n",             .color = c035},
    {.name = "Emily Anthony\n",        .color = c523},
    {.name = "Eriktronic\n",           .color = c135},
    {.name = "Greg Lord (gplord)\n",   .color = c045},
    {.name = "J.Vega (@JVeg199X)\n",   .color = c250},
    {.name = "JFrye/Swadgeman\n",      .color = c505},
    {.name = "Joe Newman\n",           .color = c250},
    {.name = "Jonathan Moriarty\n",    .color = c215},
    {.name = "Kaitie Lawson\n",        .color = c550},
    {.name = "Kevin \"PF3k\" Lin\n",   .color = c225},
    {.name = "Livingston Rampey\n",    .color = c205},
    {.name = "MrTroy\n",               .color = c555},
    {.name = "Nilly (@VanillyNeko)\n", .color = c424},
    {.name = "silvershoelaces\n",      .color = c315},
    {.name = "Socks\n",                .color = c450},
    {.name = "Steven Hewitt\n",        .color = c541},
    {.name = "thaeli\n",               .color = c245},
    {.name = "timeshifter08\n",        .color = c144},
    {.name = "",                       .color = c000},
    {.name = "",                       .color = c000},
};
// clang-format on

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
    initCredits(credits, creditsFont, entries, ARRAY_SIZE(entries));
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
