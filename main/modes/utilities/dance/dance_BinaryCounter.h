#pragma once

#include <stdint.h>
#include "dance.h"

void danceBinaryCounter(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Counts up on LEDS in binary.
 * The 'on' color is smoothly iterated over the color wheel. The 'off'
 * color is also iterated over the color wheel, 180 degrees offset from 'on'
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceBinaryCounter(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static int32_t binaryCount       = 0;
    static uint32_t binaryCountTimer = 0;

    if (reset)
    {
        binaryCount      = 0;
        binaryCountTimer = 1000000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    // Run timer every 1s
    RUN_TIMER_EVERY(binaryCountTimer, 1000000, tElapsedUs, {
        // Increment count
        binaryCount++;

        // Pick colors
        int16_t angle     = binaryCount % 256;
        uint32_t colorOn  = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);
        uint32_t colorOff = EHSVtoHEXhelper((angle + 128) % 256, 0xFF, 0xFF, false);

        // Set each LED
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (binaryCount & (1 << i))
            {
                leds[i].r = (colorOn >> 0) & 0xFF;
                leds[i].g = (colorOn >> 8) & 0xFF;
                leds[i].b = (colorOn >> 16) & 0xFF;
            }
            else
            {
                leds[i].r = (colorOff >> 0) & 0xFF;
                leds[i].g = (colorOff >> 8) & 0xFF;
                leds[i].b = (colorOff >> 16) & 0xFF;
            }
        }

        // Mark LEDs as updated
        ledsUpdated = true;
    });

    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
