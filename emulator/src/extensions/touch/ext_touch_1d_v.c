//==============================================================================
// Imports
//==============================================================================

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "macros.h"
#include "trigonometry.h"

#include "ext_touch_1d_v.h"

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

static bool touch_1DV_Init(emuArgs_t* emuArgs);
static int32_t touch_1DV_Key(uint32_t key, bool down, modKey_t modifiers);
static bool touch_1DV_MouseMove(int32_t x, int32_t y, mouseButton_t buttonMask);
static bool touch_1DV_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down);
static void touch_1DV_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes);
static bool isInBounds(int32_t* x, int32_t* y);
static void calcCirclePoly(RDPoint* buf, uint32_t tris, uint32_t xo, uint32_t yo, uint32_t r);

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    bool clicked;
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

emuExtension_t touchEmu1DVExtension = {
    .name            = "touch_1DV",
    .fnInitCb        = touch_1DV_Init,
    .fnDeinitCb      = NULL,
    .fnPreFrameCb    = NULL,
    .fnPostFrameCb   = NULL,
    .fnKeyCb         = touch_1DV_Key,
    .fnMouseMoveCb   = touch_1DV_MouseMove,
    .fnMouseButtonCb = touch_1DV_MouseButton,
    .fnRenderCb      = touch_1DV_Render,
};

static emuTouch_t emuTouch1DV = {0};

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
static bool touch_1DV_Init(emuArgs_t* emuArgs)
{
    emuTouch1DV.clicked   = false;
    emuTouch1DV.mouseX    = -1;
    emuTouch1DV.mouseY    = -1;
    emuTouch1DV.intensity = INTENSITY_MIN;

    if (emuArgs->emulateTouch)
    {
        requestPane(&touchEmu1DVExtension, PANE_RIGHT, PANE_MIN_SIZE, PANE_MIN_SIZE);
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
    if (emuTouch1DV.paneX <= *x && *x < emuTouch1DV.paneX + emuTouch1DV.paneW)
    {
        if (emuTouch1DV.paneY <= *y && *y < emuTouch1DV.paneY + emuTouch1DV.paneH)
        {
            *x -= emuTouch1DV.paneX;
            *y -= emuTouch1DV.paneY;
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
static bool updateTouch_1DV(int32_t x, int32_t y, bool clicked)
{
    if (isInBounds(&x, &y))
    {
        if (emuTouch1DV.clicked && !clicked)
        {
            // TODO notify Swadge
            printf("Touch 1D V release\n");
        }

        emuTouch1DV.clicked = clicked;
        emuTouch1DV.mouseX  = x;
        emuTouch1DV.mouseY  = y;

        if (clicked)
        {
            // TODO notify Swadge
            printf("Touch 1D V %" PRId32 " (%" PRId32 ") \n", (emuTouch1DV.mouseY * 1024) / emuTouch1DV.paneH,
                   emuTouch1DV.intensity);
        }
        return true;
    }
    else
    {
        if (emuTouch1DV.clicked)
        {
            // TODO notify Swadge
            printf("Touch 1D V release\n");
        }
        emuTouch1DV.clicked = false;
        emuTouch1DV.mouseX  = -1;
        emuTouch1DV.mouseY  = -1;

        // TODO notify Swadge
        return false;
    }
}

/**
 * @brief TODO doc
 *
 * @param key
 * @param down
 * @param modifiers
 * @return int32_t
 */
static int32_t touch_1DV_Key(uint32_t key, bool down, modKey_t modifiers)
{
    // TODO fix '0'
    int32_t keyDigit = key - '6';

    if (keyDigit < 0 || keyDigit >= NUM_KEY_ZONES || modifiers != EMU_MOD_NONE)
    {
        // Do not consume event, we only want 6 to 0 without a modifier
        return 0;
    }

    // Calculate the key bit to update internal state
    uint32_t keyBit = 1 << keyDigit;

    // Calculate area for simulated touch
    int32_t zoneH = emuTouch1DV.paneH / NUM_KEY_ZONES;
    int32_t simX  = emuTouch1DV.paneX + (emuTouch1DV.paneW / 2);
    int32_t simY  = emuTouch1DV.paneY + (zoneH / 2) + (keyDigit * zoneH);

    // Process simulated touch
    if (down && !(emuTouch1DV.keyState & keyBit))
    {
        emuTouch1DV.keyState |= keyBit;
        updateTouch_1DV(simX, simY, true);
    }
    else if (emuTouch1DV.keyState & keyBit)
    {
        emuTouch1DV.keyState &= ~keyBit;
        updateTouch_1DV(simX, simY, false);
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
static bool touch_1DV_MouseMove(int32_t x, int32_t y, mouseButton_t buttonMask)
{
    return updateTouch_1DV(x, y, (buttonMask & EMU_MOUSE_LEFT) == EMU_MOUSE_LEFT);
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
static bool touch_1DV_MouseButton(int32_t x, int32_t y, mouseButton_t button, bool down)
{
    if (button == EMU_MOUSE_LEFT)
    {
        return updateTouch_1DV(x, y, down);
    }
    else if (button == EMU_SCROLL_UP)
    {
        emuTouch1DV.intensity = CLAMP(emuTouch1DV.intensity + 1024, INTENSITY_MIN, INTENSITY_MAX);
        return true;
    }
    else if (button == EMU_SCROLL_DOWN)
    {
        emuTouch1DV.intensity = CLAMP(emuTouch1DV.intensity - 1024, INTENSITY_MIN, INTENSITY_MAX);
        return true;
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
static void touch_1DV_Render(uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes)
{
    // We only should have one pane, so just exit if we don't have it
    if (numPanes < 1)
    {
        return;
    }

    // Save the pane dimensions for later
    emuTouch1DV.paneX = pane->paneX;
    emuTouch1DV.paneW = pane->paneW;
    emuTouch1DV.paneH = pane->paneH;
    emuTouch1DV.paneY = pane->paneY;

    CNFGColor(COLOR_BG);
    CNFGTackRectangle(pane->paneX, pane->paneY, pane->paneX + pane->paneW, pane->paneY + pane->paneH);

    if (emuTouch1DV.mouseX >= 0 && emuTouch1DV.mouseY >= 0)
    {
        // Draw a circle for the touch location
        RDPoint points[36];
        uint32_t r = 8 + ((24 * (emuTouch1DV.intensity - INTENSITY_MIN)) / INTENSITY_MAX);
        uint32_t x = emuTouch1DV.paneX + emuTouch1DV.mouseX;
        uint32_t y = emuTouch1DV.paneY + emuTouch1DV.mouseY;
        calcCirclePoly(points, ARRAY_SIZE(points), x, y, r);

        if (emuTouch1DV.clicked)
        {
            CNFGColor(COLOR_TOUCH);
        }
        else
        {
            CNFGColor(COLOR_HOVER);
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
