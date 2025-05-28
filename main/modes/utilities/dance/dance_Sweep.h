#pragma once

#include <stdint.h>
#include "dance.h"

void danceSweep(uint32_t tElapsedUs, uint32_t arg, bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * @brief Sweep the LEDs left and right, like a Cylon
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param arg        The color for this sweep
 * @param reset      true to reset this dance's variables
 */
void danceSweep(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static const int8_t ledOrder[][2] = {
        {3, 5}, {4, -1}, {6, -1}, {2, -1}, {7, -1}, {0, -1}, {1, 8},
    };

    static int32_t sweepTimer                      = 0;
    static int32_t stripExciter                    = 0;
    static int16_t stripVals[ARRAY_SIZE(ledOrder)] = {0};
    static bool stripDir                           = true;
    static int32_t rgbAngle                        = 0;

    if (reset)
    {
        sweepTimer   = 0;
        stripExciter = 0;
        memset(stripVals, 0, sizeof(stripVals));
        stripDir = true;
        rgbAngle = 0;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    RUN_TIMER_EVERY(sweepTimer, DEFAULT_FRAME_RATE_US, tElapsedUs, {
        // Run an exciter to lead the strip
        int8_t stripIdx = stripExciter / 8;
        if (stripExciter % 8 == 0 && stripIdx < ARRAY_SIZE(ledOrder))
        {
            stripVals[stripIdx] = 0xFF;
        }

        // Flip directions at the end
        if (stripIdx < 0 || stripIdx >= ARRAY_SIZE(ledOrder))
        {
            stripDir = !stripDir;
        }

        // Move the exciter
        if (stripDir)
        {
            stripExciter++;
        }
        else
        {
            stripExciter--;
        }

        // Apply rainbow if there's no color
        if (0 == arg)
        {
            arg = EHSVtoHEXhelper(rgbAngle, 0xFF, 0xFF, false);
            rgbAngle++;
            if (256 == rgbAngle)
            {
                rgbAngle = 0;
            }
        }

        // Decay the strip
        for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(ledOrder); sIdx++)
        {
            stripVals[sIdx] -= 8;
            if (stripVals[sIdx] < 0)
            {
                stripVals[sIdx] = 0;
            }

            for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(ledOrder[0]); lIdx++)
            {
                int8_t numLed = ledOrder[sIdx][lIdx];
                if (0 <= numLed)
                {
                    leds[ledOrder[sIdx][lIdx]].r = (stripVals[sIdx] * ARG_R(arg)) / 256;
                    leds[ledOrder[sIdx][lIdx]].g = (stripVals[sIdx] * ARG_G(arg)) / 256;
                    leds[ledOrder[sIdx][lIdx]].b = (stripVals[sIdx] * ARG_B(arg)) / 256;
                }
            }
            ledsUpdated = true;
        }
    });

    // Light the LEDs
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
