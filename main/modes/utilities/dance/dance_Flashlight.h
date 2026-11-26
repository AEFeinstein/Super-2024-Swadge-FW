#pragma once

#include <stdint.h>
#include "dance.h"

void danceFlashlight(uint32_t tElapsedUs, uint32_t arg, bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Turn on all LEDs and Make Purely White
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceFlashlight(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        tAccumulated = 70000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 70000)
    {
        tAccumulated -= 70000;
        ledsUpdated = true;

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r = ARG_R(arg);
            leds[i].g = ARG_G(arg);
            leds[i].b = ARG_B(arg);
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
