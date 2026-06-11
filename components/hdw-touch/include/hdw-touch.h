/*! \file hdw-touch.h
 *
 * \section touch_design Design Philosophy
 *
 * This component handles touch pads, which are touch-sensitive areas on the PCB.
 *
 * The individual touch pads can be represented as a single circular analog touch pad which reports touches in polar
 * coordinates with intensity or multiple linear touch strips which report 1D position and intensity. The hardware will
 * dictate how touch pads are represented.
 *
 * \section t_pad_design Touch Pad Design Philosophy
 *
 * Touch hardware has changed between years, so differently shaped touch pads can be configured either with
 * initTouchLinear() (multiple 1D touch strips) or initTouchJoystick() (single circular 2D touch pad). Swadge modes
 * should not use readings from the physically discrete touch pads.
 *
 * Touch pads are not polled. Events like touches are not queued to be processed later. The individual Swadge mode must
 * poll the current touch state with getTouchJoystick() or getTouchLinear(). Both of those functions will report the
 * position and intensity of the touch.
 *
 * If touches are polled from a shape that was not initialized, no data will be returned.
 *
 * Touch pad areas are set up and read with <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.2.7/esp32s2/api-reference/peripherals/touch_pad.html">Touch
 * Sensor</a>.
 *
 * \section touch_usage Usage
 *
 * You don't need to call initTouchPads() or deinitTouchPads(). The system does at the appropriate times. Likewise,
 * initTouchLinear() or initTouchJoystick() will be called during system initialization depending on the hardware
 * configuration.
 *
 * If configured, you may call getTouchJoystick() to get the analog touch position. Three utility functions are provided
 * in touchUtils.h to interpret touch data different ways.
 * - getTouchJoystickZones() is available to translate the analog touches into a four, five, eight, or nine-way virtual
 * directional pad.
 * - getTouchSpins() is available to count the number of times the touch joystick was circled around.
 * - getTouchCartesian() is available to translate the polar coordinates of the touch into the Cartesian X-Y plane
 *
 * If configured, you may call getTouchLinear() to get get the analog touch positions. There are no utilities for extra
 * processing of 1D data.
 *
 * \section touch_example Example
 *
 * \code{.c}
 * // Check if the circular touch area is touched, and print values if it is
 * int32_t phi, r, intensity;
 * if (getTouchJoystick(&phi, &r, &intensity))
 * {
 *     ESP_LOGI("TCH", "touch center: %" PRIu32 ", intensity: %" PRIu32 ", intensity %" PRIu32, phi, r, intensity);
 * }
 * else
 * {
 *     ESP_LOGI("TCH", "no touch");
 * }
 *
 * linearTouch_t touches[2] = {0};
 * getTouchLinear(touches, ARRAY_SIZE(touches));
 * for (uint8_t tIdx = 0; tIdx < ARRAY_SIZE(touches); tIdx++)
 * {
 *     if (touches[tIdx].touched)
 *     {
 *         ESP_LOGI("TCH", "Touch %" PRIu8 " at %" PRId32 ", intensity %" PRId32, tIdx, touches[tIdx].position,
 *                  touches[tIdx].intensity);
 *     }
 *     else
 *     {
 *         ESP_LOGI("TCH", "No Touch on %" PRIu8, tIdx);
 *     }
 * }
 * \endcode
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <driver/touch_pad.h>

/**
 * @brief The configuration for a linear set of touch pads
 */
typedef struct
{
    uint8_t numTouchPads;        ///< The number of touch pads in the linear configuration
    const uint8_t* touchPadIdxs; ///< A list of touch pad indices into `touchPads[]` given to initTouchPads()
} touchLinearCfg_t;

/**
 * @brief The result of a linear touch
 */
typedef struct
{
    int32_t position;  ///< The position of the touch, from 0 to 1023
    int32_t intensity; ///< How hard the touch is being pressed
    bool touched;      ///< true if a touch is registered, false if it isn't
} linearTouch_t;

void initTouchPads(const touch_pad_t* touchPads, uint8_t numTouchPads, float touchPadSensitivity, bool denoiseEnable);
void deinitTouchPads(void);
void powerUpTouchPads(void);
void powerDownTouchPads(void);

void initTouchJoystick(uint8_t centerPadIdx, const uint8_t* ringPadIdxs);
bool getTouchJoystick(int32_t* phi, int32_t* r, int32_t* intensity);

void initTouchLinear(const touchLinearCfg_t* touchLinearCfgs, uint8_t numTouchLinearCfgs);
uint8_t getTouchLinear(linearTouch_t* touches, uint8_t numLinearTouches);
