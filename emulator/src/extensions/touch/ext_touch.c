//==============================================================================
// Imports
//==============================================================================

#include "ext_touch.h"
#include "macros.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "emu_ext.h"
#include "emu_args.h"
#include "rawdraw_sf.h"

#include "hdw-btn.h"
#include "hdw-btn_emu.h"

//==============================================================================
// Defines
//==============================================================================

// A nice gray for inactive touchpads
#define COLOR_TOUCHPAD_INACTIVE 0x337722FF
// Some sort of dark yellow, I think, for active touchpads
#define COLOR_TOUCHPAD_ACTIVE   0x88FF00FF
// Like almost gray, but a bit green?
#define COLOR_TOUCHPAD_SEP      0x336633FF

#define TOUCHPAD_SPACING 4
#define TOUCHPAD_COUNT   5

#define TOUCH_STRIP
//#define TOUCH_STICK

//==============================================================================
// Function Prototypes
//==============================================================================

static bool isInTouchBounds(int32_t x, int32_t y);
static void calculateTouch(int32_t x, int32_t y, int16_t* centroid, int16_t* intensity);
static bool updateTouch(int32_t x, int32_t y, bool clicked);

static bool touchInit(emuArgs_t* emuArgs);
static bool touchMouseMove(int32_t x, int32_t y, mouseButton_t buttonMask);
static bool touchMouseButton(int32_t x, int32_t y, mouseButton_t button, bool down);
static void touchRender(uint32_t winW, uint32_t winH, uint32_t paneW, uint32_t paneH, uint32_t paneX, uint32_t paneY);

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    emuArgs_t* emuArgs;
    bool dragging;

    int32_t lastClickX;
    int32_t lastClickY;

    bool hovering;
    int32_t lastHoverX;
    int32_t lastHoverY;

    int16_t lastTouchCentroid;
    int16_t lastTouchIntensity;
    buttonBit_t lastTouchButtons;

    uint32_t paneX;
    uint32_t paneY;
    uint32_t paneW;
    uint32_t paneH;
} emuTouch_t;

//==============================================================================
// Variables
//==============================================================================

emuCallback_t touchEmuCallback = {
    .name            = "touch",
    .paneLocation    = PANE_BOTTOM,
    .minPaneW        = 192,
    .minPaneH        = 64,
    .fnInitCb        = touchInit,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = NULL,
    .fnMouseMoveCb   = touchMouseMove,
    .fnMouseButtonCb = touchMouseButton,
    .fnRenderCb      = touchRender,
};

static emuTouch_t emuTouch = {0};

//==============================================================================
// Functions
//==============================================================================

static bool isInTouchBounds(int32_t x, int32_t y)
{
    return emuTouch.paneX <= x && x <= emuTouch.paneX + emuTouch.paneW
           && emuTouch.paneY <= y && y <= emuTouch.paneY + emuTouch.paneH;
}

static void calculateTouch(int32_t x, int32_t y, int16_t* centroid, int16_t* intensity)
{
    // Clamp the touch offset to within the pane bounds, in case we're still clicking but left the box
    uint32_t xOffset = (x < emuTouch.paneX) ? 0 : MIN(x - emuTouch.paneX, emuTouch.paneW);
    uint32_t yOffset = (y < emuTouch.paneY) ? 0 : MIN(y - emuTouch.paneY, emuTouch.paneH);

    // Just do a linear interpolation to get the magnitude and centroid

    // The touch controls go right-to-left, I think? TODO compare with real swadge
    *centroid = CLAMP(1024 - xOffset * 1024 / emuTouch.paneX, 0, 1023);
    // Towards the top of the pane, higher intensity
    *intensity = CLAMP(1024 - (yOffset * 1024 / emuTouch.paneY), 0, 1023);
}

/**
 * @brief Calculates updated touch parameters based on a click or drag location
 *
 * @param x The x mouse location
 * @param y The y mouse location
 * @param clicked True if the left-mouse button is clicked, false otherwise
 * @return true  If the event should be consumed
 * @return false If the event should not be consumed
 */
static bool updateTouch(int32_t x, int32_t y, bool clicked)
{
    bool inBounds = isInTouchBounds(x, y);

    // Are we currently clicking
    if (clicked)
    {
        // Yes, we are clicking!

        // Are we in our pane bounds and not already dragging?
        if (!emuTouch.dragging && inBounds)
        {
            // Yes, so we should be dragging now!
            emuTouch.dragging = true;
        }
    }
    else
    {
        // We're not clicking

        // So, if we're in-bounds, we're hovering
        emuTouch.hovering = inBounds;

        // And if we're hovering ,update the hover location
        if (inBounds)
        {
            emuTouch.lastHoverX = x;
            emuTouch.lastHoverY = y;
        }
    }

    // Now, regardless of the current click state, we need to handle the drag.
    // If we are clicking, we need to update the touch position.
    // If we're not clicking, we need to cancel the touch.
    if (emuTouch.dragging)
    {
        if (clicked)
        {
            // We are starting or continuing a drag
            // Save the last click location
            emuTouch.lastClickX = x;
            emuTouch.lastClickY = y;

            // Calculate the new touch values and save them
            calculateTouch(x, y, &(emuTouch.lastTouchCentroid), &(emuTouch.lastTouchIntensity));
            emulatorSetTouchCentroid(emuTouch.lastTouchCentroid, emuTouch.lastTouchIntensity);
        }
        else
        {
            // We are ending a click/drag!
            emuTouch.dragging = false;

            // ... and reset the touch centroid to 0, since we're not touching the touchpad
            emulatorSetTouchCentroid(0, 0);
        }

        // Consume this mouse event, since did something with it
        return true;
    }

    // Event was ignored, don't consume
    return false;
}

/**
 * @brief Initializes the touchpad extension.
 *
 * @param emuArgs
 * @return true
 * @return false
 */
static bool touchInit(emuArgs_t* emuArgs)
{
    emuTouch.emuArgs = emuArgs;

    emuTouch.dragging = false;
    emuTouch.lastClickX = 0;
    emuTouch.lastClickY = 0;

    return emuArgs->emulateTouch;
}

static bool touchMouseMove(int32_t x, int32_t y, mouseButton_t buttonMask)
{
    return updateTouch(x, y, (buttonMask & EMU_MOUSE_LEFT) == EMU_MOUSE_LEFT);
}

static bool touchMouseButton(int32_t x, int32_t y, mouseButton_t button, bool down)
{
    // Return true only for left-button events that actually generate a change
    return (button == EMU_MOUSE_LEFT) && updateTouch(x, y, down);
}

static void touchRender(uint32_t winW, uint32_t winH, uint32_t paneW, uint32_t paneH, uint32_t paneX, uint32_t paneY)
{
    // Save the pane dimensions for later
    emuTouch.paneX = paneX;
    emuTouch.paneW = paneW;
    emuTouch.paneH = paneH;
    emuTouch.paneY = paneY;

    // Get the current button state and filter out the touch values
    buttonBit_t buttonState = emulatorGetButtonState();
    buttonBit_t touchBits = buttonState & (TB_0 | TB_1 | TB_2 | TB_3 | TB_4);

    for (int i = 0; i < TOUCHPAD_COUNT; i++)
    {
        bool first = (i == 0);
        bool last = ((i + 1) == TOUCHPAD_COUNT);

        // Horizontal space available for pads after taking dividers into account
        uint32_t padSpace = (paneW - TOUCHPAD_SPACING * (TOUCHPAD_COUNT - 1));

        // Calculate the pad X first
        int padX = paneX + i * (TOUCHPAD_SPACING + padSpace) / TOUCHPAD_COUNT;

        // Calculate the pad width by first calculating the next pad X and subtracting this pad's X from that
        // Then calculate the pad width as the distance to the next pad X (minus spacing)
        int padW = paneX + (i + 1) * (TOUCHPAD_SPACING + padSpace) / TOUCHPAD_COUNT - (last ? TOUCHPAD_SPACING : 0) - padX;

        if (!first)
        {
            // Draw the divider to the left of this touchpad
            CNFGColor(COLOR_TOUCHPAD_SEP);
            CNFGTackRectangle(padX - TOUCHPAD_SPACING, paneY, padX - 1, paneY + paneH);
        }

        // Draw the touchpad now
        // Set the color based on whether the touchpad is considered active
        CNFGColor((touchBits & (TB_4 >> i)) ? COLOR_TOUCHPAD_ACTIVE : COLOR_TOUCHPAD_INACTIVE);
        CNFGTackRectangle(padX, paneY, padX + padW, paneY + paneH);
    }
}
