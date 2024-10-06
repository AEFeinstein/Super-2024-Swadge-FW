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

#define MIN_LED_HEIGHT 64

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
    {.x = 4, .y = 1}, {.x = 5, .y = 0}, {.x = 3, .y = 0}, {.x = 1, .y = 0}, {.x = 2, .y = 1},
    {.x = 0, .y = 2}, {.x = 2, .y = 2}, {.x = 4, .y = 2}, {.x = 6, .y = 2},
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
        requestPane(&ledEmuExtension, PANE_TOP, 1, MIN_LED_HEIGHT);
        requestPane(&ledEmuExtension, PANE_BOTTOM, 1, MIN_LED_HEIGHT * 2);
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
            int16_t xOffset, yOffset, ledW, ledH;
            // Use top pane for offset 0, bottom for offsets 1 & 2
            if (0 == ledOffsets[i].y)
            {
                ledH = panes[0].paneH;
                ledW = panes[0].paneW / 4;

                xOffset = panes[0].paneX + (ledOffsets[i].x * ledW) / 2;
                yOffset = panes[0].paneY + (ledOffsets[i].y * ledH);
            }
            else
            {
                ledH = panes[1].paneH / 2;
                ledW = panes[1].paneW / 4;

                xOffset = panes[1].paneX + (ledOffsets[i].x * ledW) / 2;
                yOffset = panes[1].paneY + ((ledOffsets[i].y - 1) * ledH);
            }
            CNFGColor((leds[i].r << 24) | (leds[i].g << 16) | (leds[i].b << 8) | 0xFF);
            CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
        }
    }
}
