//==============================================================================
// Includes
//==============================================================================

#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "hdw-btn.h"
#include "hdw-btn_emu.h"
#include "emu_main.h"
#include "linked_list.h"

//==============================================================================
// Variables
//==============================================================================

/// The keyboard keys used for input
static const char inputKeys[] = {
    'w', ///< ::PB_UP
    's', ///< ::PB_DOWN
    'a', ///< ::PB_LEFT
    'd', ///< ::PB_RIGHT
    'l', ///< ::PB_A
    'k', ///< ::PB_B
    'o', ///< ::PB_START
    'i', ///< ::PB_SELECT
    '1', ///< ::TB_0
    '2', ///< ::TB_1
    '3', ///< ::TB_2
    '4', ///< ::TB_3
    '5'  ///< ::TB_4
};

/// The current state of all input buttons
static uint32_t buttonState = 0;

/// The queue for button events
static list_t* buttonQueue;

/// The touchpad analog location
static int32_t lastTouchLoc = 0;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Initialize both pushbuttons and touch buttons
 *
 * @param pushButtons A list of GPIOs with pushbuttons to initialize. The list should be in the same order as
 * ::buttonBit_t, starting at ::PB_UP
 * @param numPushButtons The number of pushbuttons to initialize
 * @param touchPads A list of touch buttons to initialize. The list should be in the same order as ::buttonBit_t,
 * starting at ::TB_0
 * @param numTouchPads The number of touch buttons to initialize
 */
void initButtons(gpio_num_t* pushButtons, uint8_t numPushButtons, touch_pad_t* touchPads, uint8_t numTouchPads)
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
 * @brief Get totally raw touch sensor values from buffer.
 * NOTE: You must have touch callbacks enabled to use this.
 *
 * @param[out] centerVal pointer to centroid of touch locaiton from 0..1024 inclusive. Cannot be NULL.
 * @param[out] intensityVal intensity of touch press. Cannot be NULL.
 * @return true if touched (centroid), false if not touched (no centroid)
 */
bool getTouchCentroid(int32_t* centerVal, int32_t* intensityVal)
{
    if (buttonState & (TB_0 | TB_1 | TB_2 | TB_3 | TB_4))
    {
        *centerVal    = lastTouchLoc;
        *intensityVal = 512;
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief This handles key events from rawdraw
 *
 * @param keycode The key that was pressed or released
 * @param bDown true if the key was pressed, false if it was released
 */
void emulatorHandleKeys(int keycode, int bDown)
{
    // Check keycode against initialized keys
    for (uint8_t idx = 0; idx < ARRAY_SIZE(inputKeys); idx++)
    {
        // If this matches
        if (keycode == inputKeys[idx])
        {
            // Set or clear the button
            if (bDown)
            {
                // Check if button was already pressed
                if (buttonState & (1 << idx))
                {
                    // It was, just return
                    return;
                }
                else
                {
                    // It wasn't, set it!
                    buttonState |= (1 << idx);
                }
            }
            else
            {
                // Check if button was already released
                if (0 == (buttonState & (1 << idx)))
                {
                    // It was, just return
                    return;
                }
                else
                {
                    // It wasn't, clear it!
                    buttonState &= ~(1 << idx);
                }
            }

            // Create a new event
            buttonEvt_t* evt = malloc(sizeof(buttonEvt_t));
            evt->button      = (1 << idx);
            evt->down        = bDown;
            evt->state       = buttonState;

            // Add the event to the list
            push(buttonQueue, evt);
            break;
        }
    }

    /* LUT the location */
    const uint8_t touchLoc[] = {
        128, // 00000
        0,   // 00001
        64,  // 00010
        32,  // 00011
        128, // 00100
        64,  // 00101
        96,  // 00110
        64,  // 00111
        192, // 01000
        96,  // 01001
        128, // 01010
        85,  // 01011
        160, // 01100
        106, // 01101
        128, // 01110
        96,  // 01111
        255, // 10000
        128, // 10001
        160, // 10010
        106, // 10011
        192, // 10100
        128, // 10101
        149, // 10110
        112, // 10111
        224, // 11000
        149, // 11001
        170, // 11010
        128, // 11011
        192, // 11100
        144, // 11101
        160, // 11110
        128, // 11111
    };

    // The bottom 8 bits are pushbuttons, followed by five touch buttons
    int touchState = ((buttonState >> 8) & 0x1F);
    lastTouchLoc   = touchLoc[touchState];
}
