/*! \file hdw-touch.h
 *
 * \section touch_design Design Philosophy
 *
 * This component handles touch pads, which are touch-sensitive areas on the PCB.
 *
 * The individual touch pads are only represented as a single, larger, circular analog touchpad which reports touches in
 * polar coordinates.
 *
 * \section t_pad_design Touch-pad Design Philosophy
 *
 * Touch pads are treated as a single circular area (not discrete touch areas) and are not
 * polled. Events like touches are not queued to be processed later. The individual Swadge mode must poll the current
 * touch state with getTouchJoystick(). The touch state reports the polar coordinates of the touch (angle and radius) as
 * well as the intensity of the touch.
 *
 * Touch-pad areas are set up and read with <a
 * href="https://docs.espressif.com/projects/esp-idf/en/v5.2.7/esp32s2/api-reference/peripherals/touch_pad.html">Touch
 * Sensor</a>.
 *
 * \section touch_usage Usage
 *
 * You don't need to call initTouchPads() or deinitTouchPads(). The system does at the appropriate times.
 *
 * You may call getTouchJoystick() to get the analog touch position.
 * Three utility functions are provided in touchUtils.h to interpret touch data different ways.
 * - getTouchJoystickZones() is available to translate the analog touches into a four, five, eight, or nine-way virtual
 * directional pad.
 * - getTouchSpins() is available to count the number of times the touch joystick was circled around.
 * - getTouchCartesian() is available to translate the polar coordinates of the touch into the Cartesian X-Y plane
 *
 * \section touch_example Example
 *
 * \code{.c}
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
