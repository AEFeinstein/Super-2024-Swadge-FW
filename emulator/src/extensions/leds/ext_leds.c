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
        requestPane(&ledEmuExtension, PANE_TOP, MIN_LED_DIM * 3, MIN_LED_DIM);
        requestPane(&ledEmuExtension, PANE_BOTTOM, MIN_LED_DIM * 3, MIN_LED_DIM);
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
    // If we don't have 4 panes, just exit.
    if (numPanes < 4)
    {
        return;
    }

    uint8_t numLeds;
    led_t* leds = getLedMemory(&numLeds);

    // Get convenience pointers to panes
    const emuPane_t* lPane = &panes[0];
    const emuPane_t* rPane = &panes[1];
    const emuPane_t* tPane = &panes[2];
    const emuPane_t* bPane = &panes[3];

    // Do some math to find the size of the border around the screen
    int32_t borderY = tPane->paneH / 2;
    int32_t borderH = rPane->paneH - (tPane->paneH / 2) - (bPane->paneH / 2);

    // These variables get re-used for LEDs
    const led_t* led = NULL;
    int32_t xOffset  = 0;
    int32_t yOffset  = 0;
    int32_t ledW;
    int32_t ledH;

    ///////////////////////////////////////////////////////////////////////
    // Body LEDs, top pane

    ledW    = tPane->paneW / 4;
    ledH    = tPane->paneH / 2;
    xOffset = tPane->paneX;
    yOffset = tPane->paneY + (tPane->paneH / 2);

    led = &leds[1];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
    xOffset += ledW;

    led = &leds[0];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + (ledW * 2), yOffset + ledH);
    xOffset += (ledW * 2);

    led = &leds[7];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);

    ///////////////////////////////////////////////////////////////////////
    // Body LEDs, right pane

    ledW    = rPane->paneW / 2;
    ledH    = borderH / 4;
    xOffset = rPane->paneX;
    yOffset = rPane->paneY + borderY;

    led = &leds[7];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
    yOffset += ledH;

    led = &leds[6];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + (ledH * 2));
    yOffset += (ledH * 2);

    led = &leds[5];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);

    ///////////////////////////////////////////////////////////////////////
    // Body LEDs, bottom pane

    ledW    = bPane->paneW / 4;
    ledH    = bPane->paneH / 2;
    xOffset = bPane->paneX;
    yOffset = bPane->paneY;

    led = &leds[3];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
    xOffset += ledW;

    led = &leds[4];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + (ledW * 2), yOffset + ledH);
    xOffset += (ledW * 2);

    led = &leds[5];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);

    ///////////////////////////////////////////////////////////////////////
    // Body LEDs, left pane

    ledW    = lPane->paneW / 2;
    ledH    = borderH / 4;
    xOffset = lPane->paneX + (lPane->paneW / 2);
    yOffset = lPane->paneY + borderY;

    led = &leds[1];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
    yOffset += ledH;

    led = &leds[2];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + (ledH * 2));
    yOffset += (ledH * 2);

    led = &leds[3];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);

    ///////////////////////////////////////////////////////////////////////
    // Wing LEDs, right side

    ledW    = rPane->paneW / 2;
    ledH    = rPane->paneH / 3;
    xOffset = rPane->paneX + (rPane->paneW / 2);
    yOffset = rPane->paneY;

    led = &leds[8];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
    yOffset += ledH;

    led = &leds[9];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
    yOffset += ledH;

    led = &leds[10];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);

    ///////////////////////////////////////////////////////////////////////
    // Wing LEDs, left side

    ledW    = lPane->paneW / 2;
    ledH    = lPane->paneH / 3;
    xOffset = lPane->paneX;
    yOffset = lPane->paneY;

    led = &leds[13];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
    yOffset += ledH;

    led = &leds[12];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
    yOffset += ledH;

    led = &leds[11];
    CNFGColor((led->r << 24) | (led->g << 16) | (led->b << 8) | 0xFF);
    CNFGTackRectangle(xOffset, yOffset, xOffset + ledW, yOffset + ledH);
}
