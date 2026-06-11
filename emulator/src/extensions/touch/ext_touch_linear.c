//==============================================================================
// Imports
//==============================================================================

#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "macros.h"
#include "trigonometry.h"

#include "hdw-touch_emu.h"
#include "ext_touch_linear.h"

//==============================================================================
// Defines
//==============================================================================

// Background color
#define COLOR_BG 0x336633FF
// Some sort of dark yellow, I think, for active touch
#define COLOR_TOUCH 0x88FF00FF
// Some light gray for hover, maybe transparent??
#define COLOR_HOVER 0x44884488

// These come from just playing around with a real swadge
#define INTENSITY_MAX (1 << 18)
#define INTENSITY_MIN (1 << 10)

//==============================================================================
// Function Prototypes
//==============================================================================

static bool updateTouchLinear(emuTouch_t* et, int32_t x, int32_t y, mouseButton_t clicked);
static bool isInBounds(emuTouch_t* et, int32_t* x, int32_t* y);
static void calcCirclePoly(RDPoint* buf, uint32_t tris, uint32_t xo, uint32_t yo, uint32_t r);

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initializes a linear touchpad extension. If emulateTouch is set, panes for the UI will be requested
 *
 * @param et The data to initialize
 * @param ext The extension to initialize
 * @param emuArgs The arguments to use for initialization
 * @param isHorz true for a horizontal touchpad, false for a vertical one
 * @param keys A list of keyboard keys to use for simulated touch. This memory is not copied, so it must be accessible
 * for the lifetime of the extension
 * @param numKeys The number of keyboard keys used for simulated touch
 * @return true if the extension was initialized, false otherwise
 */
bool touchLinearInit(emuTouch_t* et, emuExtension_t* ext, const emuArgs_t* emuArgs, bool isHorz, const char* keys,
                     uint8_t numKeys)
{
    et->isHorz = isHorz;

    // Setup mouse
    et->clickState = NOT_CLICKED;
    et->mouseX     = -1;
    et->mouseY     = -1;
    et->intensity  = INTENSITY_MIN + (INTENSITY_MAX - INTENSITY_MIN) / 2;

    // Setup keyboard
    et->keys    = keys;
    et->numKeys = numKeys;

    if (emuArgs->emulateTouch)
    {
        if (isHorz)
        {
            requestPane(ext, PANE_BOTTOM, TOUCH_PANE_MIN_SIZE, TOUCH_PANE_MIN_SIZE);
        }
        else
        {
            requestPane(ext, PANE_RIGHT, TOUCH_PANE_MIN_SIZE, TOUCH_PANE_MIN_SIZE);
        }
    }

    return true;
}

/**
 * @brief Check if an event is in bounds for this extension and translate the coordinates to pane-local from
 * screen-local
 *
 * @param et The touch extension data
 * @param x The X location of the event, will be translated if in-bounds
 * @param y The Y location of the event, will be translated if in-bounds
 * @return true if the event was in-bounds, false otherwise
 */
static bool isInBounds(emuTouch_t* et, int32_t* x, int32_t* y)
{
    if (et->paneX <= *x && *x < et->paneX + et->paneW)
    {
        if (et->paneY <= *y && *y < et->paneY + et->paneH)
        {
            // Translate from screen coordinates to pane coordinates
            *x -= et->paneX;
            *y -= et->paneY;
            return true;
        }
    }
    return false;
}

/**
 * @brief Calculates updated touch parameters based on a click or drag location
 *
 * @param et The touch extension data
 * @param x The x mouse location
 * @param y The y mouse location
 * @param clicked The mouse button which was clicked
 * @return true  If the event was consumed, false otherwise
 */
static bool updateTouchLinear(emuTouch_t* et, int32_t x, int32_t y, mouseButton_t clicked)
{
    if (isInBounds(et, &x, &y))
    {
        // Update click state
        if ((EMU_MOUSE_LEFT == clicked) || (EMU_MOUSE_RIGHT == clicked))
        {
            // Update mouse state
            et->clickState = (EMU_MOUSE_LEFT == clicked) ? LEFT_CLICKED : RIGHT_CLICKED;
            // Pressed down
            if (et->isHorz)
            {
                emulatorSetTouchLinear(0, (et->mouseX * 1024) / et->paneW, et->intensity);
            }
            else
            {
                emulatorSetTouchLinear(1, (et->mouseY * 1024) / et->paneH, et->intensity);
            }
        }
        else if (EMU_MOUSE_NONE == clicked)
        {
            if (LEFT_CLICKED == et->clickState)
            {
                // Update mouse state
                et->clickState = LEFT_RELEASED;
                // Release
                emulatorSetTouchLinear(et->isHorz ? 0 : 1, 0, 0);
            }
            else if (RIGHT_CLICKED == et->clickState)
            {
                // Update mouse state
                et->clickState = RIGHT_RELEASED;
                // Don't release the touch when the right mouse button is released
            }
        }

        // Update position, except for the RIGHT_RELEASED state, which stays locked
        if (RIGHT_RELEASED != et->clickState)
        {
            et->mouseX = x;
            et->mouseY = y;
        }

        // In bounds, consumed
        return true;
    }

    // Out of bounds
    return false;
}

/**
 * @brief Handle a keyboard key for simulated touch
 *
 * @param et The touch extension data
 * @param key The key which was pressed
 * @param down True if the key is down, false if the key is up
 * @param modifiers Any modifiers for this keystroke
 * @return -1 if the key was consumed, 0 otherwise
 */
int32_t touchLinearKey(emuTouch_t* et, uint32_t key, bool down, modKey_t modifiers)
{
    if (modifiers != EMU_MOD_NONE)
    {
        // Don't consume keys with modifiers
        return 0;
    }

    int8_t keyDigit = -1;
    for (uint8_t kIdx = 0; kIdx < et->numKeys; kIdx++)
    {
        if (et->keys[kIdx] == key)
        {
            keyDigit = kIdx;
        }
    }

    if (keyDigit < 0)
    {
        // Do not consume event, didn't match a key
        return 0;
    }

    // Calculate the key bit to update internal state
    uint32_t keyBit = 1 << keyDigit;

    // Calculate location for simulated touch
    int32_t simX = 0;
    int32_t simY = 0;
    if (et->isHorz)
    {
        int32_t zoneW = et->paneW / et->numKeys;
        simX          = et->paneX + (zoneW / 2) + (keyDigit * zoneW);
        simY          = et->paneY + (et->paneH / 2);
    }
    else
    {
        int32_t zoneH = et->paneH / et->numKeys;
        simX          = et->paneX + (et->paneW / 2);
        simY          = et->paneY + (zoneH / 2) + (keyDigit * zoneH);
    }

    // Process simulated touch
    if (down && !(et->keyState & keyBit))
    {
        et->keyState |= keyBit;
        updateTouchLinear(et, simX, simY, true);
    }
    else if (et->keyState & keyBit)
    {
        et->keyState &= ~keyBit;
        updateTouchLinear(et, simX, simY, false);
    }

    // Consume event
    return -1;
}

/**
 * @brief Handle mouse movement for simulated touch
 *
 * @param et The touch extension data
 * @param x The x mouse location
 * @param y The y mouse location
 * @param buttonMask A bitmask of all mouse buttons currently held (ignored)
 * @return true if the event was consumed, false if it wasn't
 * @return false
 */
bool touchLinearMouseMove(emuTouch_t* et, int32_t x, int32_t y, mouseBit_t buttonMask)
{
    // Translate current buttonMask to bitmask.
    // Note that buttonMask is ignored
    mouseBit_t button = EMU_MOUSE_BIT_NONE;
    if (LEFT_CLICKED == et->clickState)
    {
        button = EMU_MOUSE_BIT_LEFT;
    }
    else if (RIGHT_CLICKED == et->clickState)
    {
        button = EMU_MOUSE_BIT_RIGHT;
    }

    // Update the touch
    return updateTouchLinear(et, x, y, button);
}

/**
 * @brief Handle mouse button events for simulated touch
 *
 * @param et The touch extension data
 * @param x The x mouse location
 * @param y The y mouse location
 * @param button The mouse button which had an event
 * @param down True if the mouse button was clicked, false if it was released
 * @return true if the event was consumed, false otherwise
 */
bool touchLinearMouseButton(emuTouch_t* et, int32_t x, int32_t y, mouseButton_t button, bool down)
{
    switch (button)
    {
        case EMU_MOUSE_NONE:
        case EMU_MOUSE_MIDDLE:
        case EMU_SCROLL_LEFT:
        case EMU_SCROLL_RIGHT:
        default:
        {
            // Not consumed
            return false;
        }
        case EMU_MOUSE_LEFT:
        case EMU_MOUSE_RIGHT:
        case EMU_MOUSE_THUMB_1:
        case EMU_MOUSE_THUMB_2:
        {
            if (EMU_MOUSE_THUMB_1 == button)
            {
                // Thumb 1 is a light touch
                et->intensity = INTENSITY_MIN;
                button        = EMU_MOUSE_LEFT;
            }
            else if (EMU_MOUSE_THUMB_2 == button)
            {
                // Thumb 2 is a heavy touch
                et->intensity = INTENSITY_MAX;
                button        = EMU_MOUSE_LEFT;
            }

            // Process the button click
            return updateTouchLinear(et, x, y, down ? button : EMU_MOUSE_NONE);
        }
        case EMU_SCROLL_UP:
        {
            if (isInBounds(et, &x, &y))
            {
                // Increase intensity
                et->intensity = CLAMP(et->intensity + 1024, INTENSITY_MIN, INTENSITY_MAX);
                return true;
            }
            break;
        }
        case EMU_SCROLL_DOWN:
        {
            if (isInBounds(et, &x, &y))
            {
                // Decrease intensity
                et->intensity = CLAMP(et->intensity - 1024, INTENSITY_MIN, INTENSITY_MAX);
                return true;
            }
            break;
        }
    }

    return false;
}

/**
 * @brief Render the pane for simulated touch
 *
 * @param et The touch extension data
 * @param winW The window width in pixels
 * @param winH The window height in pixels
 * @param pane The pane to render into
 * @param numPanes
 */
void touchLinearRender(emuTouch_t* et, uint32_t winW, uint32_t winH, const emuPane_t* pane, uint8_t numPanes)
{
    // We only should have one pane, so just exit if we don't have it
    if (numPanes < 1)
    {
        return;
    }

    // Save the pane dimensions for later
    et->paneX = pane->paneX;
    et->paneW = pane->paneW;
    et->paneH = pane->paneH;
    et->paneY = pane->paneY;

    // Draw the background
    CNFGColor(COLOR_BG);
    CNFGTackRectangle(pane->paneX, pane->paneY, pane->paneX + pane->paneW, pane->paneY + pane->paneH);

    // If the mouse is in bounds
    if (et->mouseX >= 0 && et->mouseY >= 0)
    {
        // Draw a circle for the touch location
        RDPoint points[36];
        uint32_t r = 8 + ((24 * (et->intensity - INTENSITY_MIN)) / INTENSITY_MAX);
        uint32_t x = et->paneX + et->mouseX;
        uint32_t y = et->paneY + et->mouseY;
        calcCirclePoly(points, ARRAY_SIZE(points), x, y, r);

        // Pick color based on state
        switch (et->clickState)
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
        // Draw circle
        CNFGTackPoly(points, ARRAY_SIZE(points));

        // Draw a line
        if (et->isHorz)
        {
            CNFGTackSegment(x, et->paneY, x, et->paneY + et->paneH);
        }
        else
        {
            CNFGTackSegment(et->paneX, y, et->paneX + et->paneW, y);
        }
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
static void calcCirclePoly(RDPoint* buf, uint32_t tris, uint32_t xo, uint32_t yo, uint32_t r)
{
    for (int i = 0; i < (tris); i++)
    {
        buf[i].x = (xo) + getCos1024(i * 360 / (tris)) * (r) / 1024;
        buf[i].y = (yo) + getSin1024(i * 360 / (tris)) * (r) / 1024;
    }
}
