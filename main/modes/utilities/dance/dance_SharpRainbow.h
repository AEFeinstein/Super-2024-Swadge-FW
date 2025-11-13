#pragma once

#include <stdint.h>
#include "dance.h"

void danceSharpRainbow(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Sharply rotate a color wheel around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceSharpRainbow(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static int32_t ledCount      = 0;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        ledCount     = 0;
        tAccumulated = 300000;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 300000)
    {
        tAccumulated -= 300000;
        ledsUpdated = true;

        ledCount = ledCount + 1;
        if (ledCount > CONFIG_NUM_LEDS - 1)
        {
            ledCount = 0;
        }

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            int16_t angle  = (((i * 256) / CONFIG_NUM_LEDS)) % 256;
            uint32_t color = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

            leds[(i + ledCount) % CONFIG_NUM_LEDS].r = (color >> 0) & 0xFF;
            leds[(i + ledCount) % CONFIG_NUM_LEDS].g = (color >> 8) & 0xFF;
            leds[(i + ledCount) % CONFIG_NUM_LEDS].b = (color >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
