#pragma once

#include <stdint.h>
#include "dance.h"

void danceSmoothRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Smoothly rotate a color wheel around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceSmoothRainbow(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    // Map comet index to LED index
    const int8_t ledMap[CONFIG_NUM_LEDS] = {0, 2, 1, 4, 3, 5};

    static uint32_t tAccumulated = 0;
    static uint8_t ledCount      = 0;

    if (reset)
    {
        ledCount     = 0;
        tAccumulated = arg;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= arg)
    {
        tAccumulated -= arg;
        ledsUpdated = true;

        ledCount--;

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            int16_t angle  = ((((i * 256) / CONFIG_NUM_LEDS)) + ledCount) % 256;
            uint32_t color = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

            leds[ledMap[i]].r = (color >> 0) & 0xFF;
            leds[ledMap[i]].g = (color >> 8) & 0xFF;
            leds[ledMap[i]].b = (color >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
