#pragma once

#include <stdint.h>
#include "dance.h"

void dancePoliceSiren(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * police sirens, flash half red then half blue
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void dancePoliceSiren(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static bool sideLit;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        sideLit      = false;
        tAccumulated = 500000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 500000)
    {
        tAccumulated -= 500000;
        ledsUpdated = true;

        // Alternate which side is lit
        sideLit = !sideLit;

        // These are the LEDs on each side
        static const uint8_t halves[2][5] = {
            {0, 1, 2, 7, 8},
            {2, 3, 4, 5, 6},
        };

        // These are the colors for each side
        static const led_t colors[2] = {
            {
                .r = 0xFF,
                .g = 0x00,
                .b = 0x00,
            },
            {
                .r = 0x00,
                .g = 0x00,
                .b = 0xFF,
            },
        };

        // Set the appropriate LEDs to the appropriate color
        for (uint32_t i = 0; i < ARRAY_SIZE(halves[0]); i++)
        {
            leds[halves[sideLit][i]] = colors[sideLit];
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
