//==============================================================================
// Imports
//==============================================================================

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "macros.h"
#include "trigonometry.h"

#include "hdw-touch_emu.h"
#include "ext_touch_1d_h.h"

//==============================================================================
// Defines
//==============================================================================

// The minimum width & height of the pane
#define PANE_MIN_SIZE 128

// Background color
#define COLOR_BG 0x336633FF
// Some sort of dark yellow, I think, for active touch
#define COLOR_TOUCH 0x88FF00FF
// Some light gray for hover, maybe transparent??
#define COLOR_HOVER 0x44884488

// These come from just playing around with a real swadge
#define INTENSITY_MAX (1 << 18)
#define INTENSITY_MIN (1 << 10)

// The number of zones corresponding to keyboard digitss
#define NUM_KEY_ZONES 5

//==============================================================================
// Function Prototypes
//==============================================================================

static bool touch_1DH_Init(emuArgs_t* emuArgs);
static int32_t touch_1DH_Key(uint32_t key, bool down, modKey_t modifiers);
static bool touch_1DH_MouseMove(int32_t x, int32_t y, mouseBit_t buttonMask);
static bool touch_1DH_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down);
static void touch_1DH_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes);
static bool isInBounds(int32_t* x, int32_t* y);
static void calcCirclePoly(RDPoint* buf, uint32_t tris, uint32_t xo, uint32_t yo, uint32_t r);

//==============================================================================
// Structs
//==============================================================================

typedef enum
{
    NOT_CLICKED,
    LEFT_CLICKED,
    LEFT_RELEASED,
    RIGHT_CLICKED,
    RIGHT_RELEASED,
} mouseClickState_t;

typedef struct
{
    mouseClickState_t clickState;
    int32_t mouseX;
    int32_t mouseY;
    int32_t intensity;
    int32_t keyState;

    uint32_t paneX;
    uint32_t paneY;
    uint32_t paneW;
    uint32_t paneH;
} emuTouch_t;

//==============================================================================
// Variables
//==============================================================================

emuExtension_t touchEmu1DHExtension = {
    .name            = "touch_1DH",
    .fnInitCb        = touch_1DH_Init,
    .fnDeinitCb      = NULL,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = touch_1DH_Key,
    .fnMouseMoveCb   = touch_1DH_MouseMove,
    .fnMouseButtonCb = touch_1DH_MouseButton,
    .fnRenderCb      = touch_1DH_Render,
};

static emuTouch_t emuTouch1DH = {0};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initializes the touchpad extension. If emulateTouch is enabled, the touchpad will display.
 *
 * Using keys 1-4 to control the touchpad 1-4 is always enabled.
 *
 * @param emuArgs
 * @return true If touchpad emulation is enabled (it is)
 */
static bool touch_1DH_Init(emuArgs_t* emuArgs)
{
    emuTouch1DH.clickState = NOT_CLICKED;
    emuTouch1DH.mouseX     = -1;
    emuTouch1DH.mouseY     = -1;
    emuTouch1DH.intensity  = INTENSITY_MIN;

    if (emuArgs->emulateTouch)
    {
        requestPane(&touchEmu1DHExtension, PANE_BOTTOM, PANE_MIN_SIZE, PANE_MIN_SIZE);
    }

    return true;
}

/**
 * @brief TODO doc
 *
 * @param x
 * @param y
 * @return true
 * @return false
 */
bool isInBounds(int32_t* x, int32_t* y)
{
    if (emuTouch1DH.paneX <= *x && *x < emuTouch1DH.paneX + emuTouch1DH.paneW)
    {
        if (emuTouch1DH.paneY <= *y && *y < emuTouch1DH.paneY + emuTouch1DH.paneH)
        {
            *x -= emuTouch1DH.paneX;
            *y -= emuTouch1DH.paneY;
            return true;
        }
    }
    return false;
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
static bool updateTouch_1DH(int32_t x, int32_t y, mouseButton_t clicked)
{
    if (isInBounds(&x, &y))
    {
        // Update click state
        if ((EMU_MOUSE_LEFT == clicked) || (EMU_MOUSE_RIGHT == clicked))
        {
            // Update mouse state
            emuTouch1DH.clickState = (EMU_MOUSE_LEFT == clicked) ? LEFT_CLICKED : RIGHT_CLICKED;
            // Pressed down
            emulatorSetTouchLinear(0, (emuTouch1DH.mouseX * 1024) / emuTouch1DH.paneW, emuTouch1DH.intensity);
        }
        else if (EMU_MOUSE_NONE == clicked)
        {
            if (LEFT_CLICKED == emuTouch1DH.clickState)
            {
                // Update mouse state
                emuTouch1DH.clickState = LEFT_RELEASED;
                // Release
                emulatorSetTouchLinear(0, 0, 0);
            }
            else if (RIGHT_CLICKED == emuTouch1DH.clickState)
            {
                // Update mouse state
                emuTouch1DH.clickState = RIGHT_RELEASED;
                // Don't release the touch when the right mouse button is released
            }
        }

        // Update position, except for the RIGHT_RELEASED state, which stays locked
        if (RIGHT_RELEASED != emuTouch1DH.clickState)
        {
            emuTouch1DH.mouseX = x;
            emuTouch1DH.mouseY = y;
        }

        // In bounds, consumed
        return true;
    }

    // Out of bounds
    return false;
}

/**
 * @brief TODO doc
 *
 * @param key
 * @param down
 * @param modifiers
 * @return int32_t
 */
static int32_t touch_1DH_Key(uint32_t key, bool down, modKey_t modifiers)
{
    int32_t keyDigit = key - '1';

    if (keyDigit < 0 || keyDigit >= NUM_KEY_ZONES || modifiers != EMU_MOD_NONE)
    {
        // Do not consume event, we only want 1 to 4 without a modifier
        return 0;
    }

    // Calculate the key bit to update internal state
    uint32_t keyBit = 1 << keyDigit;

    // Calculate area for simulated touch
    int32_t zoneW = emuTouch1DH.paneW / NUM_KEY_ZONES;
    int32_t simX  = emuTouch1DH.paneX + (zoneW / 2) + (keyDigit * zoneW);
    int32_t simY  = emuTouch1DH.paneY + (emuTouch1DH.paneH / 2);

    // Process simulated touch
    if (down && !(emuTouch1DH.keyState & keyBit))
    {
        emuTouch1DH.keyState |= keyBit;
        updateTouch_1DH(simX, simY, true);
    }
    else if (emuTouch1DH.keyState & keyBit)
    {
        emuTouch1DH.keyState &= ~keyBit;
        updateTouch_1DH(simX, simY, false);
    }

    // Consume event
    return -1;
}

/**
 * @brief TODO doc
 *
 * @param x
 * @param y
 * @param buttonMask
 * @return true
 * @return false
 */
static bool touch_1DH_MouseMove(int32_t x, int32_t y, mouseBit_t buttonMask)
{
    mouseBit_t button = EMU_MOUSE_BIT_NONE;
    if (LEFT_CLICKED == emuTouch1DH.clickState)
    {
        button = EMU_MOUSE_BIT_LEFT;
    }
    else if (RIGHT_CLICKED == emuTouch1DH.clickState)
    {
        button = EMU_MOUSE_BIT_RIGHT;
    }
    return updateTouch_1DH(x, y, button);
}

/**
 * @brief TODO doc
 *
 * @param x
 * @param y
 * @param button
 * @param down
 * @return true
 * @return false
 */
static bool touch_1DH_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down)
{
    // printf("Button %04X\n", button); // 1 thru 9
    if (button == EMU_MOUSE_LEFT || button == EMU_MOUSE_RIGHT)
    {
        return updateTouch_1DH(x, y, down ? button : EMU_MOUSE_NONE);
    }
    else if (button == EMU_SCROLL_UP)
    {
        if (isInBounds(&x, &y))
        {
            emuTouch1DH.intensity = CLAMP(emuTouch1DH.intensity + 1024, INTENSITY_MIN, INTENSITY_MAX);
            return true;
        }
    }
    else if (button == EMU_SCROLL_DOWN)
    {
        if (isInBounds(&x, &y))
        {
            emuTouch1DH.intensity = CLAMP(emuTouch1DH.intensity - 1024, INTENSITY_MIN, INTENSITY_MAX);
            return true;
        }
    }
    return false;
}

/**
 * @brief TODO doc
 *
 * @param winW
 * @param winH
 * @param pane
 * @param numPanes
 */
static void touch_1DH_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes)
{
    // We only should have one pane, so just exit if we don't have it
    if (numPanes < 1)
    {
        return;
    }

    // Save the pane dimensions for later
    emuTouch1DH.paneX = pane->paneX;
    emuTouch1DH.paneW = pane->paneW;
    emuTouch1DH.paneH = pane->paneH;
    emuTouch1DH.paneY = pane->paneY;

    CNFGColor(COLOR_BG);
    CNFGTackRectangle(pane->paneX, pane->paneY, pane->paneX + pane->paneW, pane->paneY + pane->paneH);

    if (emuTouch1DH.mouseX >= 0 && emuTouch1DH.mouseY >= 0)
    {
        // Draw a circle for the touch location
        RDPoint points[36];
        uint32_t r = 8 + ((24 * (emuTouch1DH.intensity - INTENSITY_MIN)) / INTENSITY_MAX);
        uint32_t x = emuTouch1DH.paneX + emuTouch1DH.mouseX;
        uint32_t y = emuTouch1DH.paneY + emuTouch1DH.mouseY;
        calcCirclePoly(points, ARRAY_SIZE(points), x, y, r);

        switch (emuTouch1DH.clickState)
        {
            case LEFT_CLICKED:
            case RIGHT_CLICKED:
            case RIGHT_RELEASED:
            {
                CNFGColor(COLOR_TOUCH);
                break;
            }
            default:
            {
                CNFGColor(COLOR_HOVER);
                break;
            }
        }
        CNFGTackPoly(points, ARRAY_SIZE(points));
    }
}

/**
 * @brief Generate vertices for a polygon approximating a circle with `tris` sides
 *
 * @param buf  A buffer of RDPoint[] with at least `tris` of space
 * @param tris The number of triangles to use to approximate a circle. Best if `(360 % tris) == 0`
 * @param xo   The X-offset of the center of the circle
 * @param yo   The Y-offset of the center of the circle
 * @param r    The radius of the circle
 */
void calcCirclePoly(RDPoint* buf, uint32_t tris, uint32_t xo, uint32_t yo, uint32_t r)
{
    for (int i = 0; i < (tris); i++)
    {
        buf[i].x = (xo) + getCos1024(i * 360 / (tris)) * (r) / 1024;
        buf[i].y = (yo) + getSin1024(i * 360 / (tris)) * (r) / 1024;
    }
}
