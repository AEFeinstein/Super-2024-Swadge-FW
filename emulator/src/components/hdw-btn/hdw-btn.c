//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "hdw-btn.h"
#include "hdw-btn_emu.h"
#include "emu_main.h"
#include "trigonometry.h"
#include "linked_list.h"
#include "touchUtils.h"

//==============================================================================
// Variables
//==============================================================================

/// The keyboard keys used for input
static const char inputKeys[] = {
    'W', ///< ::PB_UP
    'S', ///< ::PB_DOWN
    'A', ///< ::PB_LEFT
    'D', ///< ::PB_RIGHT
    'L', ///< ::PB_A
    'K', ///< ::PB_B
    'O', ///< ::PB_START
    'I'  ///< ::PB_SELECT
};

/// The current state of all input buttons
static uint32_t buttonState = 0;

/// The queue for button events
static list_t* buttonQueue;

/// The touchpad analog location angle
static int32_t lastTouchPhi = 0;

/// The touchpad analog location radius
static int32_t lastTouchRadius = 0;

/// The touchpad analog intensity
static int32_t lastTouchIntensity = 0;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize both pushbuttons and touch buttons
 *
 * @param pushButtons A list of GPIOs with pushbuttons to initialize. The list should be in the same order as
 * ::buttonBit_t, starting at ::PB_UP
 * @param numPushButtons The number of pushbuttons to initialize
 * @param touchPads A list of touch areas that make up a touchpad to initialize.
 * @param numTouchPads The number of touch buttons to initialize
 */
void initButtons(gpio_num_t* pushButtons, uint8_t numPushButtons, touch_pad_t* touchPads, uint8_t numTouchPads, bool flipTouch)
{
    buttonState = 0;
    buttonQueue = calloc(1, sizeof(list_t));
}

/**
 * @brief Free memory used by the buttons
 */
void deinitButtons(void)
{
    // Check the queue
    void* val;

    // No events
    while (NULL != (val = shift(buttonQueue)))
    {
        // Free everything
        free(val);
    }
    clear(buttonQueue);
    free(buttonQueue);
}

/**
 * @brief Service the queue of button events that caused interrupts
 * This only reutrns a single event, even if there are multiple in the queue
 * This function may be called multiple times in a row to completely empty the queue
 *
 * @param evt If an event occurred, return it through this argument
 * @return true if an event occurred, false if nothing happened
 */
bool checkButtonQueue(buttonEvt_t* evt)
{
    // Check the queue
    buttonEvt_t* val = shift(buttonQueue);

    // No events
    if (NULL == val)
    {
        memset(evt, 0, sizeof(buttonEvt_t));
        return false;
    }
    else
    {
        // Copy the event to the arg
        memcpy(evt, val, sizeof(buttonEvt_t));
        // Free everything
        free(val);
        // Return that an event occurred
        return true;
    }
}

/**
 * @brief Get the touch intensity and location in terms of angle and distance from
 * the center touchpad
 *
 * @param[out] phi A pointer to return the angle of the center of the touch, in degrees
 * @param[out] r A pointer to return the radius of the touch centroid
 * @param[out] intensity A pointer to return the intensity of the touch
 * @return true If the touchpad was touched and values were written to the out-params
 * @return false If no touch is detected and nothing was written
 */
int getTouchJoystick(int32_t* phi, int32_t* r, int32_t* intensity)
{
    // If lastTouchIntensity is 0, we should return false as that's "not touched"
    if (0 == lastTouchIntensity)
    {
        return false;
    }

    // A touch in the center at 50% intensity
    if (phi)
    {
        *phi = lastTouchPhi;
    }

    if (r)
    {
        *r = lastTouchRadius;
    }

    if (intensity)
    {
        *intensity = lastTouchIntensity;
    }

    return true;
}

/**
 * @brief Inject a single button press or release event into the emulator
 *
 * @param button
 * @param down
 */
void emulatorInjectButton(buttonBit_t button, bool down)
{
    // Set or clear the button
    if (down)
    {
        // Check if button was already pressed
        if (buttonState & button)
        {
            // It was, just return
            return;
        }
        else
        {
            // It wasn't, set it!
            buttonState |= button;
        }
    }
    else
    {
        // Check if button was already released
        if (0 == (buttonState & button))
        {
            // It was, just return
            return;
        }
        else
        {
            // It wasn't, clear it!
            buttonState &= ~button;
        }
    }

    // Create a new event
    buttonEvt_t* evt = malloc(sizeof(buttonEvt_t));
    evt->button      = button;
    evt->down        = down;
    evt->state       = buttonState;

    // Add the event to the list
    push(buttonQueue, evt);
}

void emulatorSetTouchJoystick(int32_t phi, int32_t radius, int32_t intensity)
{
    lastTouchPhi       = phi;
    lastTouchRadius    = radius;
    lastTouchIntensity = intensity;
}

/**
 * @brief This handles key events from rawdraw
 *
 * @param keycode The key that was pressed or released
 * @param bDown true if the key was pressed, false if it was released
 */
void emulatorHandleKeys(int keycode, int bDown)
{
    // Convert lowercase characters to their uppercase equivalents
    if ('a' <= keycode && keycode <= 'z')
    {
        keycode = (keycode - 'a' + 'A');
    }

    // Check keycode against initialized keys
    for (uint8_t idx = 0; idx < ARRAY_SIZE(inputKeys); idx++)
    {
        // If this matches one of the keycodes in the input key map
        if (keycode == inputKeys[idx])
        {
            emulatorInjectButton((buttonBit_t)(1 << idx), bDown);
            break;
        }
    }
}

buttonBit_t emulatorGetButtonState(void)
{
    return buttonState;
}
