#include "ext_leds.h"
#include "emu_ext.h"
#include "hdw-led.h"
#include "hdw-led_emu.h"
#include "macros.h"

#include "rawdraw_sf.h"

//==============================================================================
// Defines
//==============================================================================

#define MIN_LED_WIDTH 64

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
static const int16_t ledOffsets[8][2] = {
    {1, 2}, {0, 3}, {0, 1}, {1, 0}, // Left side LEDs
    {2, 0}, {3, 1}, {3, 3}, {2, 2}, // Right side LEDs
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initializes the
 *
 * @param args
 * @return true if the extension is enabled
 * @return true if the extension is not
 */
static bool ledsExtInit(emuArgs_t* args)
{
    if (!args->hideLeds)
    {
        requestPane(&ledEmuExtension, PANE_LEFT, MIN_LED_WIDTH * 2, 1);
        requestPane(&ledEmuExtension, PANE_RIGHT, MIN_LED_WIDTH * 2, 1);
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
            // Use left pane for offsets 0 and 1, right for offsets 2 and 3
            const emuPane_t* pane = (ledOffsets[i][0] < 2) ? (panes + 0) : (panes + 1);

            int16_t ledH = pane->paneH / (numLeds / 2);
            int16_t ledW = pane->paneW / 2;

            int16_t xOffset = pane->paneX + (ledOffsets[i][0] % 2) * (ledW / 2);
            int16_t yOffset = ledOffsets[i][1] * ledH;

            CNFGColor((leds[i].r << 24) | (leds[i].g << 16) | (leds[i].b << 8) | 0xFF);
            CNFGTackRectangle(xOffset, yOffset, xOffset + ledW * 3 / 2, yOffset + ledH);
        }
    }
}
