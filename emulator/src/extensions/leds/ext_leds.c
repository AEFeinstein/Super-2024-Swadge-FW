#include "ext_leds.h"
#include "emu_ext.h"
#include "hdw-led.h"
#include "hdw-led_emu.h"
#include "macros.h"
#include "vector2d.h"

#include "rawdraw_sf.h"

//==============================================================================
// Defines
//==============================================================================

#define MIN_LED_DIM 64
#define LED_MARGIN  16

//==============================================================================
// Static Function Prototypes
//==============================================================================

static bool ledsExtInit(emuArgs_t* args);
static void drawLeds(uint32_t winW, uint32_t winH, const emuPane_t* panes, uint8_t numPanes);

//==============================================================================
// Variables
//==============================================================================

emuExtension_t ledEmuExtension = {
    .name            = "leds",
    .fnInitCb        = ledsExtInit,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = NULL,
    .fnMouseButtonCb = NULL,
    .fnRenderCb      = drawLeds,
};

// Where LEDs are drawn, kinda
// first value is the LED column (top-to-bottom(?))
// second value is the row
static const vec_t ledOffsets[CONFIG_NUM_LEDS] = {
    {.x = 1, .y = 0}, // 1
    {.x = 1, .y = 2}, // 2
    {.x = 1, .y = 1}, // 3
    {.x = 0, .y = 1}, // 4
    {.x = 0, .y = 2}, // 5
    {.x = 0, .y = 0}, // 6
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initializes the LED panes
 *
 * @param args
 * @return true if the extension is enabled
 * @return true if the extension is not
 */
static bool ledsExtInit(emuArgs_t* args)
{
    if (!args->hideLeds)
    {
        requestPane(&ledEmuExtension, PANE_LEFT, MIN_LED_DIM, MIN_LED_DIM * 3);
        requestPane(&ledEmuExtension, PANE_RIGHT, MIN_LED_DIM, MIN_LED_DIM * 3);
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
static void drawLeds(uint32_t winW, uint32_t winH, const emuPane_t* panes, uint8_t numPanes)
{
    // If we don't have 2 panes, just exit.
    if (numPanes < 2)
    {
        return;
    }

    uint8_t numLeds;
    led_t* leds = getLedMemory(&numLeds);

    // Draw simulated LEDs
    if (numLeds > 0 && NULL != leds)
    {
        for (int i = 0; i < MIN(numLeds, ARRAY_SIZE(ledOffsets)); i++)
        {
            // Use the left pane for offset 0, right pane for offset 1
            const emuPane_t* pane;
            if (0 == ledOffsets[i].x)
            {
                // Left pane
                pane = &panes[0];
            }
            else
            {
                // Right pane
                pane = &panes[1];
            }

            int16_t ledH = (pane->paneH - (2 * LED_MARGIN)) / 3;
            int16_t ledW = pane->paneW;

            int16_t xOffset = pane->paneX;
            int16_t yOffset = pane->paneY + (ledOffsets[i].y * (ledH + LED_MARGIN));

            CNFGColor((leds[i].r << 24) | (leds[i].g << 16) | (leds[i].b << 8) | 0xFF);
            CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
        }
    }
}
