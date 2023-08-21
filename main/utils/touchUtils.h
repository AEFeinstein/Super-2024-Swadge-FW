#ifndef _TOUCH_UTILS_H_
#define _TOUCH_UTILS_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Bitmask values for all the virtual joystick positions of ::getTouchJoystick()
 */
typedef enum __attribute__((packed))
{
    TB_CENTER     = 0x0100,
    TB_RIGHT      = 0x0200,
    TB_UP         = 0x0400,
    TB_LEFT       = 0x0800,
    TB_DOWN       = 0x1000,
    TB_UP_RIGHT   = 0x0600,
    TB_UP_LEFT    = 0x0C00,
    TB_DOWN_LEFT  = 0x1800,
    TB_DOWN_RIGHT = 0x1200,
} touchJoystick_t;

typedef struct
{
    bool startSet;
    int32_t startAngle;
    int32_t startRadius;

    int32_t lastAngle;
    int32_t lastRadius;

    int32_t spins;     ///< The number of complete CCW spins made. Negative indicates clockwise spins
    int32_t remainder; ///< The angle of the partial CCW spin, or negative for a CW spin
} touchSpinState_t;

void getTouchCartesian(int32_t angle, int32_t radius, int32_t* x, int32_t* y);
touchJoystick_t getTouchJoystick(int32_t angle, int32_t radius, bool useCenter, bool useDiagonals);
void getTouchSpins(touchSpinState_t* state, int32_t angle, int32_t radius);

#endif
