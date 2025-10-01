#include <stdio.h>
#include <stddef.h>
#include "ext_led_eyes.h"
#include "hdw-ch32v003.h"

static void drawLedEyes(uint32_t winW, uint32_t winH, const emuPane_t* panes, uint8_t numPanes);
static bool ledEyesExtInit(emuArgs_t* args);

emuExtension_t ledEyesEmuExtension = {
    .name            = "ledEyes",
    .fnInitCb        = ledEyesExtInit,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = drawLedEyes,
};

#define MIN_EYE_LED_DIM 8

/**
 * @brief Initializes the LED panes
 *
 * @param args
 * @return true if the extension is enabled
 * @return true if the extension is not
 */
static bool ledEyesExtInit(emuArgs_t* args)
{
    if (!args->hideLeds)
    {
        requestPane(&ledEyesEmuExtension, PANE_TOP, MIN_EYE_LED_DIM * 13, MIN_EYE_LED_DIM * 6);
        return true;
    }

    return false;
}

/**
 * @brief Draws the LEDs onto the screen
 *
 * If \c numPanes is less than 2, this function does nothing.
 *
 * @param winH unused
 * @param winW unused
 * @param panes A list of panes to draw the LEDs in.
 * @param numPane The number of items in \c panes.
 */
static void drawLedEyes(uint32_t winW, uint32_t winH, const emuPane_t* panes, uint8_t numPanes)
{
    // If we don't have a pane, just exit.
    if (numPanes < 1)
    {
        return;
    }

    ch32v003EmuDraw(panes[0].paneX, panes[0].paneY, panes[0].paneW, panes[0].paneH);
}
