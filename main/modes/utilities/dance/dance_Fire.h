#pragma once

#include <stdint.h>
#include "dance.h"

void danceFire(uint32_t tElapsedUs, uint32_t arg, bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Fire pattern. All LEDs are random amount of red, and fifth that of green.
 * The LEDs towards the bottom have a brighter base and more randomness. The
 * LEDs towards the top are dimmer and have less randomness.
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceFire(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        tAccumulated = 100000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 75000)
    {
        tAccumulated -= 75000;
        ledsUpdated = true;

        // How bright each level flickers
        const int32_t baseLevels[][2] = {{105, 150}, {40, 24}, {16, 4}};
        // What LEDs are in each level. -1 means "no led"
        const int32_t baseLeds[][4] = {{5, 6, 7, 8}, {0, 4, -1, -1}, {1, 2, 3, -1}};

        // for each level of the fire
        for (int32_t base = 0; base < ARRAY_SIZE(baseLevels); base++)
        {
            // for each LED in that level
            for (int32_t lIdx = 0; lIdx < ARRAY_SIZE(baseLeds[0]); lIdx++)
            {
                // Get the index for convenience
                int32_t ledNum = baseLeds[base][lIdx];
                if (-1 != ledNum)
                {
                    // Randomly light the LED, within bounds
                    uint8_t randC  = danceRand(baseLevels[base][0]) + baseLevels[base][1];
                    leds[ledNum].r = (randC * ARG_R(arg)) / 256;
                    leds[ledNum].g = (randC * ARG_G(arg)) / 256;
                    leds[ledNum].b = (randC * ARG_B(arg)) / 256;
                }
            }
        }
    }
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
