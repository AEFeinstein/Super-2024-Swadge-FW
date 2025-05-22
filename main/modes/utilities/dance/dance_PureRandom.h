#pragma once

#include <stdint.h>
#include "dance.h"

void dancePureRandom(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Turn a random LED on to a random color, one at a time
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void dancePureRandom(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static uint32_t tAccumulated = 0;
    static uint8_t randLedMask   = 0;
    static uint32_t randColor    = 0;
    static uint8_t ledVal        = 0;
    static bool ledRising        = true;
    static uint32_t randInterval = 5000;

    if (reset)
    {
        randInterval = 5000;
        tAccumulated = randInterval;
        randLedMask  = 0;
        randColor    = 0;
        ledVal       = 0;
        ledRising    = true;
        return;
    }

    // Declare some LEDs, all off
    led_t leds[CONFIG_NUM_LEDS] = {{0}};
    bool ledsUpdated            = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= randInterval)
    {
        tAccumulated -= randInterval;

        if (0 == ledVal)
        {
            randColor    = danceRand(256);
            randLedMask  = danceRand(1 << CONFIG_NUM_LEDS);
            randInterval = 500 + danceRand(4096);
            ledVal++;
        }
        else if (ledRising)
        {
            ledVal++;
            if (255 == ledVal)
            {
                ledRising = false;
            }
        }
        else
        {
            ledVal--;
            if (0 == ledVal)
            {
                ledRising = true;
            }
        }

        ledsUpdated    = true;
        uint32_t color = EHSVtoHEXhelper(randColor, 0xFF, ledVal, false);
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if ((1 << i) & randLedMask)
            {
                leds[i].r = (color >> 0) & 0xFF;
                leds[i].g = (color >> 8) & 0xFF;
                leds[i].b = (color >> 16) & 0xFF;
            }
        }
    }
    // Output the LED data, actually turning them on
    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
