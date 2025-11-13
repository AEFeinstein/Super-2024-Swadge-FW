#include <stdio.h>
#include <stddef.h>
#include "ext_led_eyes.h"
#include "hdw-ch32v003.h"
#include "hdw-ch32c003_emu.h"

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
 * @brief Initializes the Eyes LED pane
 *
 * @param args Args which determine if the pane is hidden or not
 * @return true if the extension is enabled
 * @return false if the extension is not
 */
static bool ledEyesExtInit(emuArgs_t* args)
{
    if (!args->hideLeds)
    {
        return -1
               != requestPane(&ledEyesEmuExtension, PANE_TOP, MIN_EYE_LED_DIM * (EYE_LED_W + 1),
                              MIN_EYE_LED_DIM * EYE_LED_H);
    }

    return false;
}

/**
 * @brief Draws the Eyes LEDs onto the screen
 *
 * @param winH unused
 * @param winW unused
 * @param panes A list of panes to draw the LEDs in (should only be one)
 * @param numPane The number of items in \c panes (should only be one)
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
