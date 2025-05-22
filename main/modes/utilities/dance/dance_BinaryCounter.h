#pragma once

#include <stdint.h>
#include "dance.h"

void danceBinaryCounter(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Counts up to 256 in binary. At 256, the color is held for ~3s
 * The 'on' color is smoothly iterated over the color wheel. The 'off'
 * color is also iterated over the color wheel, 180 degrees offset from 'on'
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceBinaryCounter(uint32_t tElapsedUs, uint32_t arg __attribute__((unused)), bool reset)
{
    static int32_t ledCount      = 0;
    static int32_t ledCount2     = 0;
    static bool led_bool         = false;
    static uint32_t tAccumulated = 0;

    if (reset)
    {
        ledCount     = 0;
        ledCount2    = 0;
        led_bool     = false;
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

        ledCount  = ledCount + 1;
        ledCount2 = ledCount2 + 1;
        if (ledCount2 > 75)
        {
            led_bool  = !led_bool;
            ledCount2 = 0;
        }
        if (ledCount > 255)
        {
            ledCount = 0;
        }
        int16_t angle     = ledCount % 256;
        uint32_t colorOn  = EHSVtoHEXhelper(angle, 0xFF, 0xFF, false);
        uint32_t colorOff = EHSVtoHEXhelper((angle + 128) % 256, 0xFF, 0xFF, false);

        uint8_t i;
        uint8_t j;
        for (i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            if (ledCount2 >= (1 << CONFIG_NUM_LEDS))
            {
                leds[i].r = (colorOn >> 0) & 0xFF;
                leds[i].g = (colorOn >> 8) & 0xFF;
                leds[i].b = (colorOn >> 16) & 0xFF;
            }
            else
            {
                if (led_bool)
                {
                    j = CONFIG_NUM_LEDS - 1 - i;
                }
                else
                {
                    j = i;
                }

                if ((ledCount2 >> i) & 1)
                {
                    leds[(j) % CONFIG_NUM_LEDS].r = (colorOn >> 0) & 0xFF;
                    leds[(j) % CONFIG_NUM_LEDS].g = (colorOn >> 8) & 0xFF;
                    leds[(j) % CONFIG_NUM_LEDS].b = (colorOn >> 16) & 0xFF;
                }
                else
                {
                    leds[(j) % CONFIG_NUM_LEDS].r = (colorOff >> 0) & 0xFF;
                    leds[(j) % CONFIG_NUM_LEDS].g = (colorOff >> 8) & 0xFF;
                    leds[(j) % CONFIG_NUM_LEDS].b = (colorOff >> 16) & 0xFF;
                }
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
