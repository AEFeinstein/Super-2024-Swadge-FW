/*! \file hdw-btn.h
 *
 * \section btn_design Design Philosophy
 *
 * This component handles both push-buttons and touch-pads. Push-buttons are physical, tactile buttons while touch-pads
 * are touch-sensitive areas on the PCB. Events from push-buttons and touch-pads are processed different ways.
 *
 * The Swadge mode needs to call checkButtonQueueWrapper(), which calls checkButtonQueue() to receive queued button
 * events. The reason for checkButtonQueueWrapper() is so that the main loop can monitor the button which can be held
 * down to return to the main menu. The event contains which button caused the event, whether it was pressed or
 * released, and the current state of all buttons. This way the Swadge mode is not responsible for high frequency button
 * polling, and can still receive all button inputs.
 *
 * The individual touch-pads are only represented as a single, larger, circular analog touchpad which reports touches in
 * polar coordinates.
 *
 * \section pbtn_design Pushbutton Design Philosophy
 *
 * The push-buttons are polled continuously at 1ms intervals in an interrupt, but these readings are not reported to the
 * Swadge modes. The interrupt saves the prior ::DEBOUNCE_HIST_LEN polled button states and the last reported button
 * state. When all ::DEBOUNCE_HIST_LEN button states are identical, the interrupt accepts the current state and checks
 * if it different than the last reported state. If there is a difference, the button event is queued in the interrupt
 * to be received by the Swadge mode.
 *
 * The push-button GPIOs are all read at the same time using <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.2.3/esp32s2/api-reference/peripherals/dedic_gpio.html">Dedicated
 * GPIO</a>.
 *
 * Originally the push-buttons would trigger an interrupt, but we found that to have less reliable results with more
 * glitches than polling.
 *
 * Button events used to be delivered to the Swadge mode via a callback.
 * This led to cases where multiple callbacks would occur between a single invocation of that mode's main function.
 * Because the Swadge mode didn't have a separate queue for button events, this caused events to be dropped.
 * Instead of forcing each mode to queue button events, now each mode must dequeue them rather than having a callback
 * called.
 *
 * \section tpad_design Touch-pad Design Philosophy
 *
 * Unlike push-buttons, the touch-pads are treated as a single circular area (not discrete touch areas) and are not
 * polled. Events like touches are not queued to be processed later. The individual Swadge mode must poll the current
 * touch state with getTouchJoystick(). The touch state reports the polar coordinates of the touch (angle and radius) as
 * well as the intensity of the touch.
 *
 * Touch-pad areas are set up and read with <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.2.3/esp32s2/api-reference/peripherals/touch_pad.html">Touch
 * Sensor</a>.
 *
 * \section btn_usage Usage
 *
 * You don't need to call initButtons() or deinitButtons(). The system does at the appropriate times.
 *
 * You do need to call checkButtonQueueWrapper() and should do so in a while-loop to receive all events since the last
 * check. This should be done in the Swadge mode's main function.
 *
 * You may call getTouchJoystick() to get the analog touch position. This is independent of checkButtonQueueWrapper().
 * Three utility functions are provided to interpret touch data different ways.
 * - getTouchJoystickZones() is available to translate the analog touches into a four, five, eight, or nine-way virtual
 * directional pad.
 * - getTouchSpins() is available to count the number of times the touch joystick was circled around.
 * - getTouchCartesian() is available to translate the polar coordinates of the touch into the Cartesian X-Y plane
 *
 * \section btn_example Example
 *
 * \code{.c}
 * // Check all queued button events
 * buttonEvt_t evt;
 * while(checkButtonQueueWrapper(&evt))
 * {
 *     // Print the current event
 *     printf("state: %04X, button: %d, down: %s\n",
 *         evt.state, evt.button, evt.down ? "down" : "up");
 * }
 *
 * // Check if the touch area is touched, and print values if it is
 * int32_t phi, r, intensity;
 * if (getTouchJoystick(&phi, &r, &intensity))
 * {
 *     printf("touch center: %" PRIu32 ", intensity: %" PRIu32 ", intensity %" PRIu32 "\n", phi, r, intensity);
 * }
 * else
 * {
 *     printf("no touch\n");
 * }
 * \endcode
 */

#ifndef _BTN_H_
#define _BTN_H_

#include <stdbool.h>
#include <stdint.h>

#include <driver/gpio.h>
#include <driver/touch_pad.h>

/**
 * @brief Bitmask values for all the different buttons
 */
typedef enum __attribute__((packed))
{
    PB_UP     = 0x0001, //!< The up button's bit
    PB_DOWN   = 0x0002, //!< The down button's bit
    PB_LEFT   = 0x0004, //!< The left button's bit
    PB_RIGHT  = 0x0008, //!< The right button's bit
    PB_A      = 0x0010, //!< The A button's bit
    PB_B      = 0x0020, //!< The B button's bit
    PB_START  = 0x0040, //!< The start button's bit
    PB_SELECT = 0x0080, //!< The select button's bit
} buttonBit_t;

/**
 * @brief A button event containing the button that triggered the event, whether it was pressed or released, and the
 * whole button state
 */
typedef struct
{
    uint16_t state;     //!< A bitmask for the state of all buttons
    buttonBit_t button; //!< The button that caused this event
    bool down;          //!< True if the button was pressed, false if it was released
    uint32_t time;      ///!< The time of this event, in us since boot
} buttonEvt_t;

void initButtons(gpio_num_t* pushButtons, uint8_t numPushButtons, touch_pad_t* touchPads, uint8_t numTouchPads);
void deinitButtons(void);
bool checkButtonQueue(buttonEvt_t*);

int getTouchJoystick(int32_t* phi, int32_t* r, int32_t* intensity);

#endif
