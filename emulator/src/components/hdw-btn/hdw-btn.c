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

/// The default touchpad intensity value if set using keys
#define TOUCH_INTENSITY_KEY 512

// The bottom 8 bits are pushbuttons, followed by five touch buttons
#define TOUCH_BUTTON_MASK (0x1F << 8)

/// The keyboard keys used for input
static const char inputKeys[] = {
    'W', ///< ::PB_UP
    'S', ///< ::PB_DOWN
    'A', ///< ::PB_LEFT
    'D', ///< ::PB_RIGHT
    'L', ///< ::PB_A
    'K', ///< ::PB_B
    'O', ///< ::PB_START
    'I', ///< ::PB_SELECT
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

/// The touchpad analog location angle
static int32_t lastTouchAngle = 0;

/// The touchpad analog location radius
static int32_t lastTouchRadius = 0;

/// @deprecated
/// The touchpad analog location
static int32_t lastTouchLoc = 0;

/// The touchpad analog intensity
static int32_t lastTouchIntensity = 0;

//==============================================================================
// Function Prototypes
//==============================================================================

static void emulatorSetTouchCentroidOnly(int32_t centerVal, int32_t intensityVal);

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
    if (buttonState & (TB_0 | TB_1 | TB_2 | TB_3 | TB_4) || lastTouchIntensity > 0)
    {
        *centerVal    = lastTouchLoc;
        *intensityVal = lastTouchIntensity;
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Get the touch intensity and location in terms of angle and distance from
 * the center touchpad
 *
 * @param[out] angle A pointer to return the angle of the center of the touch, in degrees
 * @param[out] radius A pointer to return the radius of the touch centroid
 * @param[out] intensity A pointer to return the intensity of the touch
 * @return true If the touchpad was touched and values were written to the out-params
 * @return false If no touch is detected and nothing was written
 */
bool getTouchAngleRadius(int32_t* angle, int32_t* radius, int32_t* intensity)
{
    // If lastTouchIntensity is 0, we should return false as that's "not touched"
    // But still perform the null checks on the args like the real swadge first
    if (!angle || !radius || !intensity || 0 == lastTouchIntensity)
    {
        return false;
    }

    // Just do the actual "is the touchpad touched" check, then write placeholder values

    // TODO: Actual touchpad implementation

    // A touch in the center at 50% intensity
    *angle = lastTouchAngle;
    *radius = lastTouchRadius;
    *intensity = lastTouchIntensity;
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

void emulatorSetTouchAngleRadius(int32_t angle, int32_t radius, int32_t intensity)
{
    printf("touch(phi=%d, r=%d, i=%d)\n", angle, radius, intensity);
    lastTouchAngle = angle;
    lastTouchRadius = radius;
    lastTouchIntensity = intensity;
}

/**
 * @brief Only sets the touch centroid value without sending any events, for internal use.
 *
 * @param centerVal    The centroid center value
 * @param intensityVal The centroid intensity value
 */
static void emulatorSetTouchCentroidOnly(int32_t centerVal, int32_t intensityVal)
{
    lastTouchLoc       = centerVal;
    lastTouchIntensity = intensityVal;
}

/**
 * @brief Update the touch centroid value
 *
 * @param centerVal
 * @param intensityVal
 */
void emulatorSetTouchCentroid(int32_t centerVal, int32_t intensityVal)
{
    emulatorSetTouchCentroidOnly(centerVal, intensityVal);

    // Determine which keys are being pressed by this touch value
    // This isn't strictly a one-to-one mapping, but we're going to
    // estimate it based on the button positions

#define TOUCH_BUTTON_COUNT 5

    // The distance from centerVal that a pad would be activated
    // For the max intensity value, 1024, this would be 128, meaning that
    // any pad that intersects with [centerVal - 128, centerval + 128] would be considered touched
    // The possibility of multi-touch is ignored
    int32_t intensitySpill = (intensityVal / 8);

    int32_t touchLeft  = MAX(centerVal - intensitySpill, 0);
    int32_t touchRight = MIN(centerVal + intensitySpill, 1024);

    // We'll just send the event we expect for every button, and if the button is
    // already pressed or already released, great
    for (uint8_t touchpadNum = 0; touchpadNum < TOUCH_BUTTON_COUNT; touchpadNum++)
    {
        int32_t padLeft  = touchpadNum * 1024 / TOUCH_BUTTON_COUNT;
        int32_t padRight = (touchpadNum + 1) * 1024 / TOUCH_BUTTON_COUNT;

        buttonBit_t tb = 1 << (touchpadNum + 8);
        if (intensityVal > 0)
        {
            // There was a touch intensity value, so set all buttons according to whether the touch intersects them
            emulatorInjectButton(tb, (MAX(padLeft, touchLeft) <= MIN(padRight, touchRight)));
        }
        else
        {
            // This should be a "touchpad up" event, so set all the touchpad buttons to "released"
            emulatorInjectButton(tb, false);
        }
    }
}

/**
 * @brief Maps a bitmap of ::buttonbit_t onto a touchCentroid value.
 *
 * @param buttonState A bitmap of any ::buttonBit_t buttons. Non-touch buttons are ignored.
 * @return int32_t The corresponding touch centroid value, in the range [0, 1024)
 */
int32_t emulatorMapTouchCentroid(buttonBit_t buttonState)
{
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
    int touchState = (buttonState & TOUCH_BUTTON_MASK) >> 8;

    return touchLoc[touchState] * 4;
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
            // Check if the key we're inputting is for a touchpad button.
            // If so, we'll update the emulated touch state to match the buttons
            if (TOUCH_BUTTON_MASK & idx)
            {
                // set the centroid without creating button events, since we already handled the key presses
                emulatorSetTouchCentroidOnly(emulatorMapTouchCentroid(buttonState), TOUCH_INTENSITY_KEY);
            }

            emulatorInjectButton((buttonBit_t)(1 << idx), bDown);
            break;
        }
    }
}

buttonBit_t emulatorGetButtonState(void)
{
    return buttonState;
}