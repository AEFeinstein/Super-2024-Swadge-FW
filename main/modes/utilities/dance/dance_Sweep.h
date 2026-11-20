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
    static const int8_t ledOrder[][4] = {
        {6, 11, 12, 13},
        {0, 4, 5, -1},
        {1, 2, 3, -1},
        {7, 8, 9, 10},
    };

    static int32_t exciterTimer                    = 0;
    static int32_t exciterIdx                      = 0;
    static int16_t stripVals[ARRAY_SIZE(ledOrder)] = {0};
    static bool exciterIncrementing                = true;

    static int32_t decayTimer = 0;

    static int32_t rgbAngle = 0;

    if (reset)
    {
        exciterTimer = 0;
        exciterIdx   = 0;
        memset(stripVals, 0, sizeof(stripVals));
        exciterIncrementing = true;
        decayTimer          = 0;
        rgbAngle            = 0;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    RUN_TIMER_EVERY(exciterTimer, 250000, tElapsedUs, {
        // Excite a set of LEDs
        stripVals[exciterIdx] = 0xFF;

        // Move the exciter
        if (exciterIncrementing)
        {
            exciterIdx++;
        }
        else
        {
            exciterIdx--;
        }

        // Flip directions at the end
        if (exciterIdx < 0)
        {
            exciterIncrementing = true;
            exciterIdx          = 1;
        }
        else if (exciterIdx >= ARRAY_SIZE(ledOrder))
        {
            exciterIncrementing = false;
            exciterIdx          = ARRAY_SIZE(ledOrder) - 2;
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
    });

    // Run a timer to decay the LEDs
    RUN_TIMER_EVERY(decayTimer, 2000, tElapsedUs, {
        // Decay the strip
        for (int32_t sIdx = 0; sIdx < ARRAY_SIZE(ledOrder); sIdx++)
        {
            stripVals[sIdx]--;
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
