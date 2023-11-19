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

// The minimum width & height of the pane
#define PANE_MIN_SIZE 128

// A nice gray for inactive touchpads
#define COLOR_TOUCHPAD_INACTIVE 0x337722FF
// Like almost gray, but a bit green?
#define COLOR_TOUCHPAD_SEP 0x336633FF
// Some sort of dark yellow, I think, for active touch
#define COLOR_TOUCH 0x88FF00FF
// Some light gray for hover, maybe transparent??
#define COLOR_HOVER 0x44884488

// These come from just playing around with a real swadge
#define INTENSITY_MAX (1 << 18)
#define INTENSITY_MIN (1 << 10)

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
static void calculateTouch(int32_t x, int32_t y, int32_t* angle, int32_t* radius, int32_t* intensity);
static bool updateTouch(int32_t x, int32_t y, bool clicked);

static bool touchInit(emuArgs_t* emuArgs);
static int32_t touchKey(uint32_t key, bool down);
static bool touchMouseMove(int32_t x, int32_t y, mouseButton_t buttonMask);
static bool touchMouseButton(int32_t x, int32_t y, mouseButton_t button, bool down);
static void touchRender(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes);

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

    uint32_t keyState;

    int32_t lastTouchPhi;
    int32_t lastTouchRadius;
    int32_t lastTouchIntensity;

    uint32_t paneX;
    uint32_t paneY;
    uint32_t paneW;
    uint32_t paneH;
} emuTouch_t;

//==============================================================================
// Variables
//==============================================================================

emuExtension_t touchEmuCallback = {
    .name            = "touch",
    .fnInitCb        = touchInit,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = touchKey,
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
    int32_t xMin     = emuTouch.paneX + (emuTouch.paneW - ringSize) / 2;
    int32_t yMin     = emuTouch.paneY + (emuTouch.paneH - ringSize) / 2;

    return xMin <= x && x <= xMin + ringSize && yMin <= y && y <= yMin + ringSize;
}

static void calculateTouch(int32_t x, int32_t y, int32_t* angle, int32_t* radius, int32_t* intensity)
{
    int32_t ringSize = MIN(emuTouch.paneW, emuTouch.paneH);

    // Clamp the touch offset to within the pane bounds, in case we're still clicking but left the box
    uint32_t xOffset = (x < emuTouch.paneX) ? 0 : MIN(x - emuTouch.paneX, emuTouch.paneW);
    uint32_t yOffset = (y < emuTouch.paneY) ? 0 : MIN(y - emuTouch.paneY, emuTouch.paneH);

    // Get the x/y offsets normalized to [-1024, 1023]
    int32_t xCenterOff = CLAMP(1024 * ((int32_t)xOffset - ((int32_t)emuTouch.paneW / 2)) / (ringSize / 2), -1023, 1023);
    int32_t yCenterOff
        = CLAMP(1024 * -((int32_t)yOffset - ((int32_t)emuTouch.paneH / 2)) / (ringSize / 2), -1023, 1023);

    int32_t rawAngle = getAtan2(yCenterOff, xCenterOff);

    // convert phi to be in [0, 360) instead of [-180, 180]
    *angle = ((rawAngle % 360) + 360) % 360;

    // Calculate the radius from the offsets
    *radius = CLAMP((xCenterOff * xCenterOff + yCenterOff * yCenterOff) / 1024, 0, 1023);

    // The intensity changes elsewhere so just grab it
    *intensity = emuTouch.lastTouchIntensity;
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
            calculateTouch(x, y, &(emuTouch.lastTouchPhi), &(emuTouch.lastTouchRadius), &(emuTouch.lastTouchIntensity));
            emulatorSetTouchJoystick(emuTouch.lastTouchPhi, emuTouch.lastTouchRadius, emuTouch.lastTouchIntensity);
        }
        else
        {
            // We are ending a click/drag!
            emuTouch.dragging = false;

            // ... and reset the touch centroid to 0, since we're not touching the touchpad
            emulatorSetTouchJoystick(0, 0, 0);
        }

        // Consume this mouse event, since did something with it
        return true;
    }

    // Event was ignored, don't consume
    return false;
}

/**
 * @brief Initializes the touchpad extension. If emulateTouch is enabled, the touchpad will display.
 *
 * Using keys 1-4 to control the touchpad 1-4 is always enabled.
 *
 * @param emuArgs
 * @return true If touchpad emulation is enabled (it is)
 */
static bool touchInit(emuArgs_t* emuArgs)
{
    emuTouch.emuArgs = emuArgs;

    emuTouch.dragging   = false;
    emuTouch.lastClickX = 0;
    emuTouch.lastClickY = 0;

    emuTouch.lastTouchIntensity = INTENSITY_MIN;

    if (emuArgs->emulateTouch)
    {
        requestPane(&touchEmuCallback, PANE_BOTTOM, PANE_MIN_SIZE, PANE_MIN_SIZE);
    }

    return true;
}

static int32_t touchKey(uint32_t key, bool down)
{
    // Map from state to touchpad rotation. Button bits are in URLD order
    const int32_t phiMap[] = {
        0,   // 0b0000 ____ (0 radius)
        270, // 0b0001 ___D
        180, // 0b0010 __L_
        225, // 0b0011 __LD
        0,   // 0b0100 _R__
        315, // 0b0101 _R_D
        0,   // 0b0110 _RL_ (0 radius)
        270, // 0b0111 _RLD
        90,  // 0b1000 U___
        0,   // 0b1001 U__D (0 radius)
        135, // 0b1010 U_L_
        180, // 0b1011 U_LD
        45,  // 0b1100 UR__
        0,   // 0b1101 UR_D
        90,  // 0b1110 URL_
        0,   // 0b1111 URLD (0 radius)
    };

    if (key < '1' || key > '4')
    {
        // Do not consume event, we only want 1 to 4
        return 0;
    }

    int keyNum = (3 - (key - '1'));
    // Handle the key states separately so rolling over them works more or less how one might expect
    if (down && !(emuTouch.keyState & (1 << keyNum)))
    {
        emuTouch.keyState |= (1 << keyNum);
    }
    else if (!down && (emuTouch.keyState & (1 << keyNum)))
    {
        emuTouch.keyState &= ~(1 << keyNum);
    }

    int32_t radius    = 1024;
    int32_t intensity = emuTouch.lastTouchIntensity;
    // Check for the canceled-out positions where
    switch (emuTouch.keyState)
    {
        case 0:  // All un-pressed
        case 6:  // LR pressed
        case 9:  // UD pressed
        case 15: // All pressed
            radius    = 0;
            intensity = 0;
            break;

        default:
            break;
    }

    emulatorSetTouchJoystick(phiMap[emuTouch.keyState], radius, intensity);

    // Consume event
    return -1;
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
        emuTouch.lastTouchIntensity = CLAMP(emuTouch.lastTouchIntensity + 1024, INTENSITY_MIN, INTENSITY_MAX);
        return true;
    }
    else if (button == EMU_SCROLL_DOWN && isInTouchBounds(x, y))
    {
        emuTouch.lastTouchIntensity = CLAMP(emuTouch.lastTouchIntensity - 1024, INTENSITY_MIN, INTENSITY_MAX);
        return true;
    }
    return false;
}

static void touchRender(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes)
{
    // We only should have one pane, so just exit if we don't have it
    if (numPanes < 1)
    {
        return;
    }

    // Save the pane dimensions for later
    emuTouch.paneX = pane->paneX;
    emuTouch.paneW = pane->paneW;
    emuTouch.paneH = pane->paneH;
    emuTouch.paneY = pane->paneY;

    // outer radius of the "ring" touchpad
    uint32_t outerR = MIN(pane->paneW, pane->paneH) / 2;

    // radius of the center touchpad
    uint32_t innerR = outerR / 3;

    // radius of the circle separating the
    uint32_t spaceR = innerR + TOUCHPAD_SPACING;

    uint32_t centerX = pane->paneX + pane->paneW / 2;
    uint32_t centerY = pane->paneY + pane->paneH / 2;

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
        int32_t x = emuTouch.dragging ? (int32_t)centerX
                                            + getCos1024(emuTouch.lastTouchPhi) * emuTouch.lastTouchRadius
                                                  * (int32_t)outerR / 1024 / 1024
                                      : emuTouch.lastHoverX;
        int32_t y = emuTouch.dragging ? (int32_t)centerY
                                            - getSin1024(emuTouch.lastTouchPhi) * emuTouch.lastTouchRadius
                                                  * (int32_t)outerR / 1024 / 1024
                                      : emuTouch.lastHoverY;

        int32_t logIntensity = 0;
        int32_t tmpIntensity = emuTouch.lastTouchIntensity;
        while (tmpIntensity >>= 1)
        {
            logIntensity++;
        }

        // Draw the touch circle
        CALC_CIRCLE_POLY(points, ARRAY_SIZE(points), x, y, (logIntensity - 5) * 3 / 2);
        CNFGColor(emuTouch.dragging ? COLOR_TOUCH : COLOR_HOVER);
        CNFGTackPoly(points, ARRAY_SIZE(points));
    }
}
