/*! \file hdw-btn.h
 *
 * \section btn_design Design Philosophy
 *
 * This component handles both pushbuttons and touchpads. Pushbuttons are physical, tactile buttons while touchpads are
 * touch-sensitive areas on the PCB. Events from pushbuttons and touchpads are processed different ways, but queued in
 * the same queue.
 *
 * The Swadge Mode needs to call checkButtonQueue() to receive queued button events.
 * The event contains which button caused the event, whether it was pressed or released, and the current state of all
 * buttons. This way the Swadge Mode is not responsible for high frequency button polling, and can still receive all
 * button inputs.
 *
 * In addition to acting as binary buttons, the touchpads may act as a single, analog, touch sensitive strip. These two
 * ways of reporting are not mutually exclusive.
 *
 * \section pbtn_design Pushbutton Design Philosophy
 *
 * The pushbuttons are polled continuously at 1ms intervals in an interrupt, but these readings are not reported to the
 * Swadge modes. The interrupt saves the prior ::DEBOUNCE_HIST_LEN polled button states and the last reported button
 * state. When all ::DEBOUNCE_HIST_LEN button states are identical, the interrupt accepts the current state and checks
 * if it different than the last reported state. If there is a difference, the button event is queued in the interrupt
 * to be received by the Swadge Mode.
 *
 * The pushbutton GPIOs are all read at the same time using <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s2/api-reference/peripherals/dedic_gpio.html">Dedicated
 * GPIO</a>.
 *
 * Originally the pushbuttons would trigger an interrupt, but we found that to have glitchier and less reliable results
 * than polling.
 *
 * Button events used to be delivered to the Swadge Mode via a callback.
 * This led to cases where multiple callbacks would occur between a single invocation of that mode's main function.
 * Because the Swadge Mode didn't have a separate queue for button events, this caused events to be dropped.
 * Instead of forcing each mode to queue button events, now each mode must dequeue them rather than having a callback
 * called.
 *
 * \section tpad_design Touchpad Design Philosophy
 *
 * Unlike pushbutton polling, touchpads use interrupts to detect events. When a touchpad event is detected, an interrupt
 * will fire and the new event will be queued in the same queue used for pushbuttons.
 *
 * In addition to acting as binary buttons, the touchpad can act as a single analog touch strip. getTouchCentroid() may
 * be called to get the current analog touch value. Changes in the analog position are not reported checkButtonQueue(),
 * so getTouchCentroid() must be called as frequently as desired to get values. Do not assume that changes in the analog
 * position are correlated with events reported in checkButtonQueue().
 *
 * Touchpad interrupts are set up and touchpad values are read with <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s2/api-reference/peripherals/touch_pad.html">Touch
 * Sensor</a>.
 *
 * \section btn_usage Usage
 *
 * You don't need to call initButtons() or deinitButtons(). The system does at the appropriate times.
 *
 * You do need to call checkButtonQueue() and should do so in a while-loop to receive all events since the last check.
 * This should be done in the Swadge Mode's main function.
 *
 * You may call getTouchCentroid() to get the analog touch position. This is independent of checkButtonQueue().
 *
 * \section btn_example Example
 *
 * \code{.c}
 * #include "hdw-btn.h"
 *
 * // Check all queued button events
 * buttonEvt_t evt;
 * while(checkButtonQueue(&evt))
 * {
 *     // Print the current event
 *     printf("state: %04X, button: %d, down: %s\n",
 *         evt.state, evt.button, evt.down ? "down" : "up");
 * }
 *
 * // Check if the touch area is touched, and print values if it is
 * int32_t centerVal, intensityVal;
 * if (getTouchCentroid(&centerVal, &intensityVal))
 * {
 *     printf("touch center: %lu, intensity: %lu\n", centerVal, intensityVal);
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
    TB_0      = 0x0100, //!< Touch pad 0's button bit
    TB_1      = 0x0200, //!< Touch pad 1's button bit
    TB_2      = 0x0400, //!< Touch pad 2's button bit
    TB_3      = 0x0800, //!< Touch pad 3's button bit
    TB_4      = 0x1000, //!< Touch pad 4's button bit
} buttonBit_t;

typedef struct
{
    uint16_t state;     //!< A bitmask for the state of all buttons
    buttonBit_t button; //!< The button that caused this event
    bool down;          //!< True if the button was pressed, false if it was released
} buttonEvt_t;

void initButtons(gpio_num_t* pushButtons, uint8_t numPushButtons, touch_pad_t* touchButtons, uint8_t numTouchButtons);
void deinitButtons(void);
bool checkButtonQueue(buttonEvt_t*);

bool getTouchCentroid(int32_t* centerVal, int32_t* intensityVal);

#endif
