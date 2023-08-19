//==============================================================================
// Imports
//==============================================================================

#include "ext_touch.h"
#include "macros.h"
#include "trigonometry.h"

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
// Like almost gray, but a bit green?
#define COLOR_TOUCHPAD_SEP      0x336633FF
// Some sort of dark yellow, I think, for active touch
#define COLOR_TOUCH   0x88FF00FF
// Some light gray for hover, maybe transparent??
#define COLOR_HOVER   0x44884488

#define TOUCHPAD_SPACING 4

/**
 * @brief Macro for generating vertices for a polygon approximating a circle with `tris` sides
 *
 * @param buf  A buffer of RDPoint[] with at least `tris` of space
 * @param tris The number of triangles to use to approximate a circle. Best if `(360 % tris) == 0`
 * @param xo   The X-offset of the center of the circle
 * @param yo   The Y-offset of the center of the circle
 * @param r    The radius of the circle
 */
#define CALC_CIRCLE_POLY(buf, tris, xo, yo, r)                           \
    do                                                                   \
    {                                                                    \
        for (int i = 0; i < (tris); i++)                                 \
        {                                                                \
            buf[i].x = (xo) + getCos1024(i * 360 / (tris)) * (r) / 1024; \
            buf[i].y = (yo) + getSin1024(i * 360 / (tris)) * (r) / 1024; \
        }                                                                \
    } while (false)

//==============================================================================
// Function Prototypes
//==============================================================================

static bool isInTouchBounds(int32_t x, int32_t y);
static void calculateTouch(int32_t x, int32_t y, int16_t* angle, int16_t* radius, int16_t* intensity);
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

    int16_t lastTouchAngle;
    int16_t lastTouchRadius;
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
    .minPaneW        = 128,
    .minPaneH        = 128,
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
    int32_t ringSize = MIN(emuTouch.paneW, emuTouch.paneH);
    int32_t xMin = emuTouch.paneX + (emuTouch.paneW - ringSize) / 2;
    int32_t yMin = emuTouch.paneY + (emuTouch.paneH - ringSize) / 2;

    return xMin <= x && x <= xMin + ringSize
           && yMin <= y && y <= yMin + ringSize;
}

static void calculateTouch(int32_t x, int32_t y, int16_t* angle, int16_t* radius, int16_t* intensity)
{
    int32_t ringSize = MIN(emuTouch.paneW, emuTouch.paneH);

    // Clamp the touch offset to within the pane bounds, in case we're still clicking but left the box
    uint32_t xOffset = (x < emuTouch.paneX) ? 0 : MIN(x - emuTouch.paneX, emuTouch.paneW);
    uint32_t yOffset = (y < emuTouch.paneY) ? 0 : MIN(y - emuTouch.paneY, emuTouch.paneH);

    // Get the x/y offsets normalized to [-1024, 1023]
    int32_t xCenterOff = CLAMP(1024 * ((int32_t)xOffset - ((int32_t)emuTouch.paneW / 2)) / (ringSize / 2), -1023, 1023);
    int32_t yCenterOff = CLAMP(1024 * -((int32_t)yOffset - ((int32_t)emuTouch.paneH / 2)) / (ringSize / 2), -1023, 1023);

    int16_t rawAngle = getAtan2(yCenterOff, xCenterOff); //(int16_t)(360 * atan2(1.0 * yCenterOff, 1.0 * xCenterOff) / (2 * M_PI));

    // convert phi to be in [0, 360) instead of [-180, 180]
    *angle = ((rawAngle % 360) + 360) % 360;

    // Calculate the radius from the offsets
    *radius = CLAMP((xCenterOff * xCenterOff + yCenterOff * yCenterOff) / 1024, 0, 1023);

    // The intensity changes elsewhere so just grab it
    *intensity = CLAMP(emuTouch.lastTouchIntensity, 0, 1023);
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
            calculateTouch(x, y, &(emuTouch.lastTouchAngle), &(emuTouch.lastTouchRadius), &(emuTouch.lastTouchIntensity));
            emulatorSetTouchAngleRadius(emuTouch.lastTouchAngle, emuTouch.lastTouchRadius, emuTouch.lastTouchIntensity);
        }
        else
        {
            // We are ending a click/drag!
            emuTouch.dragging = false;

            // ... and reset the touch centroid to 0, since we're not touching the touchpad
            emulatorSetTouchAngleRadius(0, 0, 0);
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

    emuTouch.lastTouchIntensity = 512;

    return emuArgs->emulateTouch;
}

static bool touchMouseMove(int32_t x, int32_t y, mouseButton_t buttonMask)
{
    return updateTouch(x, y, (buttonMask & EMU_MOUSE_LEFT) == EMU_MOUSE_LEFT);
}

static bool touchMouseButton(int32_t x, int32_t y, mouseButton_t button, bool down)
{
    if (button == EMU_MOUSE_LEFT)
    {
        return updateTouch(x, y, down);
    }
    else if (button == EMU_SCROLL_UP && isInTouchBounds(x, y))
    {
        emuTouch.lastTouchIntensity = CLAMP(emuTouch.lastTouchIntensity << 1, 1, 1023);
        return true;
    }
    else if (button == EMU_SCROLL_DOWN && isInTouchBounds(x, y))
    {
        emuTouch.lastTouchIntensity = CLAMP((emuTouch.lastTouchIntensity + 1) >> 1, 1, 1023);
        return true;
    }
    return false;
}

static void touchRender(uint32_t winW, uint32_t winH, uint32_t paneW, uint32_t paneH, uint32_t paneX, uint32_t paneY)
{
    // Save the pane dimensions for later
    emuTouch.paneX = paneX;
    emuTouch.paneW = paneW;
    emuTouch.paneH = paneH;
    emuTouch.paneY = paneY;

    // outer radius of the "ring" touchpad
    uint32_t outerR = MIN(paneW, paneH) / 2;

    // radius of the center touchpad
    uint32_t innerR = outerR / 3;

    // radius of the circle separating the
    uint32_t spaceR = innerR + TOUCHPAD_SPACING;

    uint32_t centerX = paneX + paneW / 2;
    uint32_t centerY = paneY + paneH / 2;

    RDPoint points[32] = {0};

    // Draw the outer ring touchpad
    CALC_CIRCLE_POLY(points, ARRAY_SIZE(points), centerX, centerY, outerR);
    CNFGColor(COLOR_TOUCHPAD_INACTIVE);
    CNFGTackPoly(points, ARRAY_SIZE(points));

    // Draw the space between the center and outer touchpads
    CALC_CIRCLE_POLY(points, ARRAY_SIZE(points), centerX, centerY, spaceR);
    CNFGColor(COLOR_TOUCHPAD_SEP);
    CNFGTackPoly(points, ARRAY_SIZE(points));

    // Draw the center touchpad
    CALC_CIRCLE_POLY(points, ARRAY_SIZE(points), centerX, centerY, innerR);
    CNFGColor(COLOR_TOUCHPAD_INACTIVE);
    CNFGTackPoly(points, ARRAY_SIZE(points));

    // Draw a circle for the touch location
    // If the user is dragging anywhere, draw the circle at the location from the actual touch data
    // If the user is hovering within the touch area, draw a lighter circle underneath the mouse
    if (emuTouch.dragging || (emuTouch.hovering && isInTouchBounds(emuTouch.lastHoverX, emuTouch.lastHoverY)))
    {
        int32_t x = emuTouch.dragging ? (int32_t)centerX + getCos1024(emuTouch.lastTouchAngle) * emuTouch.lastTouchRadius * (int32_t)outerR / 1024 / 1024 : emuTouch.lastHoverX;
        int32_t y = emuTouch.dragging ? (int32_t)centerY - getSin1024(emuTouch.lastTouchAngle) * emuTouch.lastTouchRadius * (int32_t)outerR / 1024 / 1024 : emuTouch.lastHoverY;

        // Draw the touch circle
        CALC_CIRCLE_POLY(points, ARRAY_SIZE(points), x, y, emuTouch.lastTouchIntensity * innerR / 1024);
        CNFGColor(emuTouch.dragging ? COLOR_TOUCH : COLOR_HOVER);
        CNFGTackPoly(points, ARRAY_SIZE(points));
    }
}
