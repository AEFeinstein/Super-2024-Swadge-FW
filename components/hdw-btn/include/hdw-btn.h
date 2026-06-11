/*! \file hdw-btn.h
 *
 * \section btn_design Design Philosophy
 *
 * This component handles both push-buttons which are physical, tactile buttons.
 *
 * The Swadge mode needs to call checkButtonQueueWrapper(), which calls checkButtonQueue() to receive queued button
 * events. The reason for checkButtonQueueWrapper() is so that the main loop can monitor the button which can be held
 * down to return to the main menu. The event contains which button caused the event, whether it was pressed or
 * released, and the current state of all buttons. This way the Swadge mode is not responsible for high frequency button
 * polling, and can still receive all button inputs.
 *
 * \section p_btn_design Pushbutton Design Philosophy
 *
 * The push-buttons are polled continuously at 1ms intervals in an interrupt, but these readings are not reported to the
 * Swadge modes. The interrupt saves the prior ::DEBOUNCE_HIST_LEN polled button states and the last reported button
 * state. When all ::DEBOUNCE_HIST_LEN button states are identical, the interrupt accepts the current state and checks
 * if it different than the last reported state. If there is a difference, the button event is queued in the interrupt
 * to be received by the Swadge mode.
 *
 * The push-button GPIOs are all read at the same time using <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.2.7/esp32s2/api-reference/peripherals/dedic_gpio.html">Dedicated
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
 * \section btn_usage Usage
 *
 * You don't need to call initButtons() or deinitButtons(). The system does at the appropriate times.
 *
 * You do need to call checkButtonQueueWrapper() and should do so in a while-loop to receive all events since the last
 * check. This should be done in the Swadge mode's main function.
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
 * \endcode
 */

#ifndef _BTN_H_
#define _BTN_H_

#include <stdbool.h>
#include <stdint.h>

#include <driver/gpio.h>

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

void initButtons(const gpio_num_t* pushButtons, uint8_t numPushButtons);
void deinitButtons(void);
void powerDownButtons(void);
void powerUpButtons(void);
bool checkButtonQueue(buttonEvt_t*);

#endif
