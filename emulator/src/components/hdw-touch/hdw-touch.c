//==============================================================================
// Includes
//==============================================================================

#include "emu_main.h"
#include "hdw-touch.h"
#include "hdw-touch_emu.h"

//==============================================================================
// Variables
//==============================================================================

/// The touchpad analog location angle
static int32_t lastTouchPhi = 0;

/// The touchpad analog location radius
static int32_t lastTouchRadius = 0;

/// The touchpad analog intensity
static int32_t lastTouchIntensity = 0;

//==============================================================================
// Functions
//==============================================================================

void initTouchPads(const touch_pad_t* touchPads, uint8_t numTouchPads, float touchPadSensitivity, bool denoiseEnable)
{
    lastTouchPhi       = 0;
    lastTouchRadius    = 0;
    lastTouchIntensity = 0;
}

void deinitTouchPads(void)
{
    WARN_UNIMPLEMENTED();
}

void powerUpTouchPads(void)
{
    WARN_UNIMPLEMENTED();
}

void powerDownTouchPads(void)
{
    WARN_UNIMPLEMENTED();
}

void initTouchJoystick(uint8_t centerPadIdx, const uint8_t* ringPadIdxs)
{
    lastTouchPhi       = 0;
    lastTouchRadius    = 0;
    lastTouchIntensity = 0;
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
bool getTouchJoystick(int32_t* phi, int32_t* r, int32_t* intensity)
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

void emulatorSetTouchJoystick(int32_t phi, int32_t radius, int32_t intensity)
{
    lastTouchPhi       = phi;
    lastTouchRadius    = radius;
    lastTouchIntensity = intensity;
}

void initTouchLinear(const touchLinearCfg_t* touchLinearCfgs, uint8_t numTouchLinearCfgs)
{
    WARN_UNIMPLEMENTED();
}

uint8_t getTouchLinear(linearTouch_t* touches, uint8_t numLinearTouches)
{
    WARN_UNIMPLEMENTED();
    return 0;
}

void emulatorSetTouchLinear(int32_t arrIdx, int32_t position, int32_t intensity)
{
    WARN_UNIMPLEMENTED();
}