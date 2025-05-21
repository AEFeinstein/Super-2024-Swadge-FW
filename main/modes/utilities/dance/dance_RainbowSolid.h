#pragma once

#include <stdint.h>
#include "dance.h"

void danceRainbowSolid(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Turn on all LEDs and smooth iterate their singular color around the color wheel
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceRainbowSolid(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static int32_t ledCount      = 0;
    static int32_t color_save    = 0;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        ledCount     = 0;
        color_save   = 0;
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

        ledCount = ledCount + 1;
        if (ledCount > 255)
        {
            ledCount = 0;
        }
        int16_t angle = ledCount % 256;
        color_save    = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);

        uint8_t i;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            leds[i].r = (color_save >> 0) & 0xFF;
            leds[i].g = (color_save >> 8) & 0xFF;
            leds[i].b = (color_save >> 16) & 0xFF;
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
