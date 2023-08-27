#include "touchUtils.h"

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "macros.h"
#include "trigonometry.h"

/**
 * @brief Convert touchpad angle and radius to cartesian coordinates
 *
 * @param angle  The touchpad angle to convert
 * @param radius The touchpad radius to convert
 * @param[out] x A pointer to be set to the X touch coordinate, from 0 to 1023
 * @param[out] y A pointer to be set to the Y touch coordinate, from 0 to 1023
 */
void getTouchCartesian(int32_t angle, int32_t radius, int32_t* x, int32_t* y)
{
    // Set X and Y to the X and Y coords, respectively
    if (x)
    {
        *x = CLAMP((1024 + getCos1024(angle) * radius / 1024) / 2, 0, 1023);
    }

    if (y)
    {
        *y = CLAMP((1024 + getSin1024(angle) * radius / 1024) / 2, 0, 1023);
    }
}

/**
 * @brief Convert touchpad angle and radius to a joystick enum, with either 4 or 8 directions
 * and an optional center dead-zone.
 *
 * @param angle The touch angle reported by ::getTouchJoystick()
 * @param radius The touch radius reported by ::getTouchJoystick()
 * @param useCenter If true, TB_CENTER will be returned if the stick is touched but inside the dead-zone
 * @param useDiagonals If true, diagonal directions will be returned as the bitwise OR of two directions
 * @return touchJoystick_t The joystick direction, or 0 if no direction could be determined.
 */
touchJoystick_t getTouchJoystickEx(int32_t angle, int32_t radius, bool useCenter, bool useDiagonals)
{
    // Use 4 or 8 sectors, depending on whether we're using diagonals
    uint8_t sectors = useDiagonals ? 8 : 4;

    // The angle of the start of the first (RIGHT) sector
    int16_t offset = 360 - (360 / sectors) / 2;

    // Check if we're too close to the center, and return just TB_CENTER
    if (useCenter && radius < 64)
    {
        // TODO is this an OK value for the "center pad" position?
        return TB_CENTER;
    }

    // Check each sector
    for (uint8_t sector = 0; sector <= sectors; sector++)
    {
        // Divide the circle into sectors
        int16_t start = (offset + (360 * sector / sectors)) % 360;
        int16_t end   = (offset + (360 * (sector + 1) / sectors)) % 360;

        // Check if the angle is within bounds, making sure to account for wraparound
        // In the wraparound case, we just need one boundary to match, otherwise both
        if ((end < start) ? (angle < end || angle >= start) : (start <= angle && angle < end))
        {
            // Return the main button, plus (if diagonals are enabled) the next one too
            return TB_RIGHT << (sector / (useDiagonals ? 2 : 1))
                   | ((useDiagonals && (sector & 1)) ? (TB_RIGHT << ((sector / 2 + 1) % 4)) : 0);
        }
    }

    // Shouldn't happen but just in case, return 0
    return 0;
}

/**
 * @brief Calculate the number of times the touchpad was circled.
 *
 * When the touch is done, reset startSet to 0
 *
 * @param[in,out] state A pointer to a ::touchSpinState_t to use for state.
 * @param angle The current angle of the touch
 * @param radius The current radius of the touch
 */
void getTouchSpins(touchSpinState_t* state, int32_t angle, int32_t radius)
{
    if (state)
    {
        if (!state->startSet)
        {
            // This is the start of the touch
            state->startSet    = true;
            state->startAngle  = angle;
            state->startRadius = radius;

            state->lastAngle  = angle;
            state->lastRadius = radius;

            state->spins     = 0;
            state->remainder = 0;
        }
        else
        {
            // This is a continuing touch, calculate the difference
            // We assume they took the shortest path to the new value
            // Which should work great as long as we sample fast enough
            int32_t angleDiff = angle - state->lastAngle;
            if (angleDiff > 180)
            {
                // angle would be more than half a circle CCW of last turn
                // so actually, turn that into less than half of a CW turn
                angleDiff = angleDiff - 360;
            }
            else if (angleDiff < -180)
            {
                // angle would be more than half a circle CW of last turn
                // so actually, turn that into less than half of a CCW turn
                angleDiff = angleDiff + 360;
            }

            state->remainder += angleDiff;

            // Check if we just flipped the sign from negative to positive
            if (state->remainder > 0 && angleDiff > state->remainder)
            {
                if (state->spins < 0)
                {
                    state->spins++;

                    if (state->spins <= 0)
                    {
                        state->remainder -= 360;
                    }
                }
            }
            else if (state->remainder < 0 && angleDiff < state->remainder)
            {
                if (state->spins > 0)
                {
                    state->spins--;

                    if (state->spins >= 0)
                    {
                        state->remainder += 360;
                    }
                }
            }
            else if (state->remainder >= 360)
            {
                state->spins += state->remainder / 360;
                state->remainder %= 360;
            }
            else if (state->remainder <= -360)
            {
                state->spins -= state->remainder / -360;
                state->remainder %= 360;
            }

            if (state->remainder >= 360 || state->remainder <= -360)
            {
                // Pretty sure this works for both cases because of C's crazy negative modulo
                state->spins += state->remainder / 360;
                state->remainder %= 360;
            }

            state->lastAngle  = angle;
            state->lastRadius = radius;
        }
    }
}