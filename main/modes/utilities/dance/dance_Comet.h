#pragma once

#include <stdint.h>
#include "dance.h"

void danceComet(uint32_t tElapsedUs, uint32_t arg, bool reset);

#ifdef DANCE_IMPLEMENTATION

/**
 * Rotate a single white LED around the swadge
 *
 * @param tElapsedUs The time elapsed since last call, in microseconds
 * @param reset      true to reset this dance's variables
 */
void danceComet(uint32_t tElapsedUs, uint32_t arg, bool reset)
{
    // Map comet index to LED index
    const int8_t ledMap[CONFIG_NUM_LEDS] = {0, 2, 1, 4, 3, 5};

    static int32_t ledCount            = 0;
    static uint8_t rainbow             = 0;
    static int32_t msCount             = 0;
    static uint32_t tAccumulated       = 0;
    static led_t leds[CONFIG_NUM_LEDS] = {{0}};

    if (reset)
    {
        ledCount     = 0;
        rainbow      = 0;
        msCount      = 80;
        tAccumulated = 2000;
        memset(leds, 0, sizeof(leds));
        return;
    }

    bool ledsUpdated = false;

    tAccumulated += tElapsedUs;
    while (tAccumulated >= 2000)
    {
        tAccumulated -= 2000;
        for (uint8_t i = 0; i < CONFIG_NUM_LEDS; i++)
        {
            led_t* led = &leds[ledMap[i]];
            if (led->r > 0)
            {
                led->r--;
            }
            if (led->g > 0)
            {
                led->g--;
            }
            if (led->b > 0)
            {
                led->b--;
            }
        }
        msCount++;

        if (msCount % 10 == 0)
        {
            rainbow++;
        }

        if (msCount >= 80)
        {
            led_t* led = &leds[ledMap[ledCount]];
            if (0 == arg)
            {
                int32_t color = EHSVtoHEXhelper(rainbow, 0xFF, 0xFF, false);
                led->r        = (color >> 0) & 0xFF;
                led->g        = (color >> 8) & 0xFF;
                led->b        = (color >> 16) & 0xFF;
            }
            else
            {
                led->r = ARG_R(arg);
                led->g = ARG_G(arg);
                led->b = ARG_B(arg);
            }
            ledCount = (ledCount + 1) % CONFIG_NUM_LEDS;
            msCount  = 0;
        }
        ledsUpdated = true;
    }

    if (ledsUpdated)
    {
        setLeds(leds, CONFIG_NUM_LEDS);
    }
}

#endif
